#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/hwmon-sysfs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/ioctl.h>

#define DRIVER_NAME "dpll"

struct dpll_cmd {
    unsigned int reg;
    unsigned int offset;
    unsigned int val;
};

#define IOC_MAGIC 'd'

#define DPLL_READ  _IOWR(IOC_MAGIC, 1, struct dpll_cmd)
#define DPLL_WRITE _IOWR(IOC_MAGIC, 2, struct dpll_cmd)

static dev_t dpll_devt;

static struct class *dpll_sysfs_class = NULL;
#define DPLL_I2C_ADDR 0x24
#define DPLL_NUM 1

#define PAGE_OFFSET_ADDR 0x7F

#define MODE_FREERUN 0x0
#define MODE_AUTOMATIC 0x3

#define DPLL1_MAILBOX 0x02 // Bit 1 for DPLL1

#define DPLL_MODE_REFSEL_1 0x0214
#define DPLL_MON_STATUS_1  0x0119
#define DPLL_STATE_REFSEL_1 0x0120
#define DPLL_MB_MASK       0x0603
#define DPLL_MB_SEM        0x0604
#define DPLL_REF_PRIO_0    0x0614

struct dpll_dev {
    struct mutex lock;
    int port;
    bool simulate_plug_out;
    struct device *dev;
    struct i2c_client *client;
    struct cdev cdev;
    int current_page;
};

static int dpll_read(struct dpll_dev *dev, uint32_t addr, uint32_t *value) {
    int high = ((addr & 0xff) < 0x80) ? 0 : 1;
    int page = ((addr >> 8) & 0xf) * 2 + high;
    int offset = high ? ((addr & 0xff) - 0x80) : (addr & 0xff);
    int rc;
    if (dev->current_page != page) {
        rc = i2c_smbus_write_byte_data(dev->client, PAGE_OFFSET_ADDR, page);
        if (rc)
            return -1;
        dev->current_page = page;
    }
    rc = i2c_smbus_read_byte_data(dev->client, offset);
    if (rc < 0)
        return -1;
    *value = rc;
    return 0;
}

static int dpll_write(struct dpll_dev *dev, uint32_t addr, uint32_t value) {
    int high = ((addr & 0xff) < 0x80) ? 0 : 1;
    int page = ((addr >> 8) & 0xf) * 2 + high;
    int offset = high ? ((addr & 0xff) - 0x80) : (addr & 0xff);
    int rc;
    if (dev->current_page != page) {
        rc = i2c_smbus_write_byte_data(dev->client, PAGE_OFFSET_ADDR, page);
        if (rc)
            return -1;
        dev->current_page = page;
    }
    rc = i2c_smbus_write_byte_data(dev->client, offset, value);
    msleep(25);
    return rc;
}

static ssize_t index_show(struct device *dev, struct device_attribute *da, char *buf) {
    struct dpll_dev *priv = dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%d\n", priv->port);
}
static DEVICE_ATTR_RO(index);

static int get_mode(struct dpll_dev *dev) {
    int rc, v;
    rc = dpll_read(dev, DPLL_MODE_REFSEL_1, &v);
    if (rc)
        return -1;
    return v;
}

static int set_mode(struct dpll_dev *dev, int mode) {
    int current_mode;
    current_mode = get_mode(dev);
    if (current_mode < 0)
        return -1;
    if ((current_mode&0x7) == mode) {
        return 0;
    }
    current_mode &= ~(0x7);
    current_mode |= mode;
    return dpll_write(dev, DPLL_MODE_REFSEL_1, current_mode);
}

static int get_automatic_mode_status(struct dpll_dev *dev) {
    int rc, v;
    rc = dpll_read(dev, DPLL_MON_STATUS_1, &v);
    if (rc)
        return rc;
    return v;
}

static int get_prio(struct dpll_dev *dev, int pin) {
    int rc, value, offset = DPLL_REF_PRIO_0 + (pin/2);
    rc = dpll_write(dev, DPLL_MB_MASK, DPLL1_MAILBOX);
    if (rc)
        return -1;

    rc = dpll_write(dev, DPLL_MB_SEM, 0x1<<1); // bit 1 for read
    if (rc)
        return -1;

    rc = dpll_read(dev, offset, &value);
    if (rc)
        return -1;

    if (pin%2) {
        return value & 0xf;
    } else {
        return (value >> 4) & 0xf;
    }
}

static int set_prio(struct dpll_dev *dev, int pin, int prio) {
    int rc, value, offset = DPLL_REF_PRIO_0 + (pin/2);
    rc = dpll_write(dev, DPLL_MB_MASK, DPLL1_MAILBOX);
    if (rc)
        return -1;

    rc = dpll_write(dev, DPLL_MB_SEM, 0x1<<1); // bit 1 for read
    if (rc)
        return -1;

    rc = dpll_read(dev, offset, &value);
    if (rc)
        return -1;

    if (pin%2) {
        value &= ~(0xf);
        value |= prio;
    } else {
        value &= ~(0xf0);
        value |= (prio << 4);
    }
    rc = dpll_write(dev, offset, value);
    if (rc)
        return -1;
    return dpll_write(dev, DPLL_MB_SEM, 0x1);
}

static ssize_t mode_show(struct device *dev, struct device_attribute *da, char *buf) {
    int value;
    struct dpll_dev *priv = dev_get_drvdata(dev);

    mutex_lock(&priv->lock);
    value = get_mode(priv);
    mutex_unlock(&priv->lock);

    switch (value&0x7) {
    case 0:
        return scnprintf(buf, PAGE_SIZE, "Freerun\n");
    case 1:
        return scnprintf(buf, PAGE_SIZE, "Forced holdover\n");
    case 2:
        return scnprintf(buf, PAGE_SIZE, "Forced reference lock\n");
    case 3:
        return scnprintf(buf, PAGE_SIZE, "Automatic\n");
    case 4:
        return scnprintf(buf, PAGE_SIZE, "NCO\n");
    default:
        return scnprintf(buf, PAGE_SIZE, "Invalid\n");
    }
}

static ssize_t mode_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count) {
    struct dpll_dev *priv = dev_get_drvdata(dev);
    long value;

    if (kstrtoul(buf, 0, &value))
        return -EINVAL;
    if (value != 1 && value != 2) {
        printk("dpll valid modes are freerun: 1, automatic: 2\n");
        return -EINVAL;
    }

    mutex_lock(&priv->lock);
    if (set_mode(priv, (value == 1) ? MODE_FREERUN : MODE_AUTOMATIC) < 0) {
        return -1;
    }
    mutex_unlock(&priv->lock);
    return count;
}
static DEVICE_ATTR_RW(mode);

static ssize_t automatic_mode_status_show(struct device *dev, struct device_attribute *da, char *buf) {
    int value;
    struct dpll_dev *priv = dev_get_drvdata(dev);
    mutex_lock(&priv->lock);
    value = get_mode(priv);
    if ((value&0x7) != MODE_AUTOMATIC) {
        mutex_unlock(&priv->lock);
        return scnprintf(buf, PAGE_SIZE, "\n");
    }

    value = get_automatic_mode_status(priv);
    mutex_unlock(&priv->lock);

    return scnprintf(buf, PAGE_SIZE,
        "Locked: %s\n" \
        "Holdover: %s\n" \
        "Step-Time in Progress: %s\n" \
        "Holdover Ready: %s\n" \
        "At Freq Offset limit: %s\n" \
        "At PSL limit: %s\n",
        (((value>>0)&1) == 1 ? "Yes" : "No"),
        (((value>>1)&1) == 1 ? "Yes" : "No"),
        (((value>>3)&1) == 1 ? "Yes" : "No"),
        (((value>>2)&1) == 1 ? "Yes" : "No"),
        (((value>>5)&1) == 1 ? "Yes" : "No"),
        (((value>>7)&1) == 1 ? "Yes" : "No"));
}

static DEVICE_ATTR_RO(automatic_mode_status);

#define DEFINE_PRIO(name, pin) \
    static ssize_t name ## _prio_show(struct device *dev, struct device_attribute *da, char *buf) { \
        int value; \
        struct dpll_dev *priv = dev_get_drvdata(dev); \
        mutex_lock(&priv->lock); \
        value = get_prio(priv, pin); \
        mutex_unlock(&priv->lock); \
        return scnprintf(buf, PAGE_SIZE, "%d\n", value); \
    } \
    \
    static ssize_t name ## _prio_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count) { \
        struct dpll_dev *priv = dev_get_drvdata(dev); \
        long value; \
        if (kstrtoul(buf, 0, &value)) \
            return -EINVAL; \
        if (value < 0 || value > 15) { \
            printk("dpll valid prio = 0 <= prio <= 15\n"); \
            return -EINVAL; \
        } \
        mutex_lock(&priv->lock); \
        if (set_prio(priv, pin, value))\
            return -EINVAL; \
        mutex_unlock(&priv->lock); \
        return count; \
    } \
    static DEVICE_ATTR_RW(name ## _prio);

DEFINE_PRIO(ref0p, 0)
DEFINE_PRIO(ref0n, 1)
DEFINE_PRIO(ref1p, 2)
DEFINE_PRIO(ref1n, 3)
DEFINE_PRIO(ref2p, 4)
DEFINE_PRIO(ref2n, 5)
DEFINE_PRIO(ref3p, 6)
DEFINE_PRIO(ref3n, 7)

static struct attribute *dpll_dev_attrs[] = {
    &dev_attr_index.attr,
    &dev_attr_mode.attr,
    &dev_attr_automatic_mode_status.attr,
    &dev_attr_ref0p_prio.attr,
    &dev_attr_ref0n_prio.attr,
    &dev_attr_ref1p_prio.attr,
    &dev_attr_ref1n_prio.attr,
    &dev_attr_ref2p_prio.attr,
    &dev_attr_ref2n_prio.attr,
    &dev_attr_ref3p_prio.attr,
    &dev_attr_ref3n_prio.attr,
    NULL
};

ATTRIBUTE_GROUPS(dpll_dev);

long dpll_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    struct dpll_dev *dev = filp->private_data;
    long retval = 0;
    struct dpll_cmd data;

    if ( dev == NULL ) {
        printk("dev is NULL\n");
        return -EFAULT;
    }
    memset(&data, 0, sizeof(data));

    if (!capable(CAP_SYS_ADMIN)) {
        retval = -EPERM;
        goto done;
    }

    if (!access_ok((void __user *)arg, _IOC_SIZE(cmd))) {
        retval = -EFAULT;
        goto done;
    }

    if (copy_from_user(&data, (int __user *)arg, sizeof(data))) {
        retval = -EFAULT;
        goto done;
    }

    mutex_lock(&dev->lock);
    switch (cmd) {
        case DPLL_READ:
            if (dpll_read(dev, data.reg, &data.val)) {
                retval = -EFAULT;
                goto done;
            }
            if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
                retval = -EFAULT;
                goto done;
            }
            break;
        case DPLL_WRITE:
            if (dpll_write(dev, data.reg, data.val)) {
                retval = -EFAULT;
                goto done;
            }
            break;
        default:
            retval = -ENOTTY;
            break;
    }

done:
    mutex_unlock(&dev->lock);
    return retval;
}


int dpll_close(struct inode *inode, struct file *filp) {
    printk("dpll close\n");
    return 0;
}

int dpll_open(struct inode *inode, struct file *filp) {
    struct dpll_dev *priv = container_of(inode->i_cdev, struct dpll_dev, cdev);
    printk("dpll open: port %d\n", priv->port);
    filp->private_data = priv;
    return 0;
}

struct file_operations dpll_fops = {
    .owner = THIS_MODULE,
    .open = dpll_open,
    .release = dpll_close,
    .unlocked_ioctl = dpll_ioctl,
};


static int dpll_device_probe(struct i2c_client *client, const struct i2c_device_id *dev_id) {
    int ret = 0;
    struct dpll_dev *data = NULL;

    if (client->addr != DPLL_I2C_ADDR) {
        return -ENODEV;
    }

    data = kzalloc(sizeof(*data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }

    ret = sysfs_create_group(&client->dev.kobj, &dpll_dev_group);
    if (ret) {
        goto err_sysfs_create;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->lock);
    data->port = dev_id->driver_data;
    data->client = client;

    cdev_init(&data->cdev, &dpll_fops);
    data->cdev.owner = THIS_MODULE;
    data->cdev.ops = &dpll_fops;
    ret = cdev_add(&data->cdev, MKDEV(MAJOR(dpll_devt), data->port), 1);
    if (ret) {
        goto err_cdev_add;
    }

    data->dev = device_create_with_groups(dpll_sysfs_class, NULL, MKDEV(MAJOR(dpll_devt), 1), data, dpll_dev_groups, "dpll%d", data->port);
    if (IS_ERR(data->dev)) {
        ret = PTR_ERR(data->dev);
        goto err_dev_create;
    }

    return ret;

err_dev_create:
    cdev_del(&data->cdev);
err_cdev_add:
    sysfs_remove_group(&client->dev.kobj, &dpll_dev_group);
err_sysfs_create:
    kfree(data);
    return ret;
}

static int dpll_device_remove(struct i2c_client *client) {
    int ret = 0;
    struct dpll_dev *data = i2c_get_clientdata(client);
    device_destroy(dpll_sysfs_class, data->dev->devt);
    cdev_del(&data->cdev);
    sysfs_remove_group(&client->dev.kobj, &dpll_dev_group);
    kfree(data);
    return ret;
}

enum dpll_numbers {
    dpll1 = 1,
};

#define I2C_DEV_ID(x) { #x, x}

static const struct i2c_device_id dpll_device_id[] = {
I2C_DEV_ID(dpll1),
{ /* LIST END */ }
};

static struct i2c_driver dpll_driver = {
    .driver = {
        .name = DRIVER_NAME,
    },
    .probe    = dpll_device_probe,
    .remove   = dpll_device_remove,
    .id_table = dpll_device_id,
};

static int __init dpll_init(void)
{
    int rc;

    rc = alloc_chrdev_region(&dpll_devt, 0, DPLL_NUM, "dpll");
    if (rc) {
        return rc;
    }

    dpll_sysfs_class = class_create(THIS_MODULE, "dpll");
    if (IS_ERR(dpll_sysfs_class)) {
        printk("failed to create class DPLL\n");
        rc = PTR_ERR(dpll_sysfs_class);
        goto err_class_create;
    }

    rc = i2c_add_driver(&dpll_driver);
    if (rc) {
        goto err_add_driver;
    }
    return rc;

err_add_driver:
    class_destroy(dpll_sysfs_class);
err_class_create:
    unregister_chrdev_region(dpll_devt, DPLL_NUM);
    return rc;
}

static void __exit dpll_exit(void)
{
    i2c_del_driver(&dpll_driver);
    class_destroy(dpll_sysfs_class);
    unregister_chrdev_region(dpll_devt, DPLL_NUM);
}

MODULE_AUTHOR("Wataru Ishida <ishida@nel-america.com>");
MODULE_DESCRIPTION("Galileo FlexT DPLL driver");
MODULE_LICENSE("GPL");

module_init(dpll_init);
module_exit(dpll_exit);
