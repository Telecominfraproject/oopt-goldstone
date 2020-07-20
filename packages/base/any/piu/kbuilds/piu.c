#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/hwmon-sysfs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#include "piu.h"

#define DRIVER_NAME "piu"

static struct class *piu_sysfs_class = NULL;
static dev_t piu_devt;

#define PIU_I2C_ADDR 0x6A
#define PIU_NUM 8


struct piu_data {
    struct mutex lock;
    struct i2c_client *client;
    int port;
    struct cdev cdev;
    struct device *dev;
};

static ssize_t piu_index_show(struct device *dev, struct device_attribute *da, char *buf) {
    struct piu_data *priv = dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%d\n", priv->port);
}
static DEVICE_ATTR_RO(piu_index);

static ssize_t piu_type_show(struct device *dev, struct device_attribute *da, char *buf) {
    int type, ret;
    struct piu_data *priv = dev_get_drvdata(dev);
    mutex_lock(&priv->lock);
    type = i2c_smbus_read_byte_data(priv->client, 0);
    if ( type < 0 ) {
        ret = scnprintf(buf, PAGE_SIZE, "\n");
    } else if ( type == 0x10 || type == 0x11 || type == 0x12 ) {
        ret = scnprintf(buf, PAGE_SIZE, "DCO\n");
    } else if ( type == 0xff ) {
        ret = scnprintf(buf, PAGE_SIZE, "ACO\n");
    } else if ( type == 0x00 || type == 0x01 || type == 0x02 ) {
        ret = scnprintf(buf, PAGE_SIZE, "QSFP28\n");
    } else {
        ret = scnprintf(buf, PAGE_SIZE, "UNKNOWN(0x%x)\n", type);
    }
    mutex_unlock(&priv->lock);
    return ret;
}
static DEVICE_ATTR_RO(piu_type);

static struct attribute *piu_dev_attrs[] = {
    &dev_attr_piu_index.attr,
    &dev_attr_piu_type.attr,
    NULL
};
ATTRIBUTE_GROUPS(piu_dev);

#define I2C_ADDRESS_SIZE 4

static int I2C_ADDRESS_ADDRESS[] = {0x10, 0x11, 0x12, 0x13};
static int I2C_DATA_ADDRESS[] = {0x20, 0x21, 0x22, 0x23};

#define I2C_TRIGGER_ADDRESS  0x01
#define I2C_TRIGGER_WRITE 0x01
#define I2C_TRIGGER_READ 0x02

#define I2C_READY_ADDRESS 0x02
#define I2C_READY_VALUE 0x00

#define I2C_POLL_TIMEOUT 50
#define I2C_POLL_INTERVAL_US 100

static int piu_i2c_wait_ready(struct i2c_client *client) {
    int i, ret;
    for (i = 0; i < I2C_POLL_TIMEOUT; i++) {
        ret = i2c_smbus_read_byte_data(client, I2C_READY_ADDRESS);
        if ( ret < 0 ){
            return ret;
        }
        if ( ret == I2C_READY_VALUE ) {
            return 0;
        }
        msleep(I2C_POLL_INTERVAL_US);
    }
    printk("PIU I/O timeout\n");
    return -1;
}

static int piu_i2c_read(struct i2c_client *client, uint32_t addr, uint32_t *value) {
    int i, ret;
    if ( client == NULL || value == NULL ) {
        return -1;
    }
    *value = 0;
    for (i = 0; i < I2C_ADDRESS_SIZE; i++) {
        if ((ret = i2c_smbus_write_byte_data(client, I2C_ADDRESS_ADDRESS[i], (addr >> ((I2C_ADDRESS_SIZE - 1 - i)*8)) & 0xff)) < 0) {
            return ret;
        }
    }
    if ((ret = i2c_smbus_write_byte_data(client, I2C_TRIGGER_ADDRESS, I2C_TRIGGER_READ)) < 0) {
        return ret;
    }
    if ( piu_i2c_wait_ready(client) < 0 ){
        return -1;
    }
    for (i = 0; i < I2C_ADDRESS_SIZE; i++) {
        ret = i2c_smbus_read_byte_data(client, I2C_DATA_ADDRESS[i]);
        if ( ret < 0 ){
            return -1;
        }
        *value |= (ret << ((I2C_ADDRESS_SIZE - 1 - i)*8));
    }
    return 0;
}

static int piu_i2c_write(struct i2c_client *client, uint32_t addr, uint32_t value) {
    int i, ret;
    if ( client == NULL ) {
        return -1;
    }
    for (i = 0; i < I2C_ADDRESS_SIZE; i++) {
        if ((ret = i2c_smbus_write_byte_data(client, I2C_ADDRESS_ADDRESS[i], (addr >> ((I2C_ADDRESS_SIZE - 1 - i)*8)) & 0xff)) < 0) {
            return ret;
        }
    }
    for (i = 0; i < I2C_ADDRESS_SIZE; i++) {
        if ((ret = i2c_smbus_write_byte_data(client, I2C_DATA_ADDRESS[i], (value >> ((I2C_ADDRESS_SIZE - 1 - i)*8)) & 0xff)) < 0) {
            return ret;
        }
    }
    if ((ret = i2c_smbus_write_byte_data(client, I2C_TRIGGER_ADDRESS, I2C_TRIGGER_WRITE)) < 0) {
            return ret;
    }
    return piu_i2c_wait_ready(client);
}

long piu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    struct piu_data *dev = filp->private_data;
    long retval = 0;
    struct piu_cmd data;

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
        case PIU_READ:
            if (piu_i2c_read(dev->client, data.reg, &data.val)) {
                retval = -EFAULT;
                goto err;
            }
            if ( copy_to_user((int __user *)arg, &data, sizeof(data)) ) {
                retval = -EFAULT;
                goto err;
            }
            break;
        case PIU_WRITE:
            if (piu_i2c_write(dev->client, data.reg, data.val)) {
                retval = -EFAULT;
                goto err;
            }
            break;
        default:
            retval = -ENOTTY;
            break;
    }
err:
    mutex_unlock(&dev->lock);

done:
    return retval;
}

int piu_close(struct inode *inode, struct file *filp) {
    printk("close\n");
    return 0;
}

int piu_open(struct inode *inode, struct file *filp) {
    struct piu_data *priv = container_of(inode->i_cdev, struct piu_data, cdev);
    printk("open\n");
    filp->private_data = priv;
    return 0;
}

struct file_operations piu_fops = {
    .owner = THIS_MODULE,
    .open = piu_open,
    .release = piu_close,
    .unlocked_ioctl = piu_ioctl,
};

enum piu_numbers {
    piu1 = 1,
    piu2,
    piu3,
    piu4,
    piu5,
    piu6,
    piu7,
    piu8
};

#define I2C_DEV_ID(x) { #x, x}

static const struct i2c_device_id piu_device_id[] = {
I2C_DEV_ID(piu1),
I2C_DEV_ID(piu2),
I2C_DEV_ID(piu3),
I2C_DEV_ID(piu4),
I2C_DEV_ID(piu5),
I2C_DEV_ID(piu6),
I2C_DEV_ID(piu7),
I2C_DEV_ID(piu8),
{ /* LIST END */ }
};

/* Platform dependent +++ */
static int piu_device_probe(struct i2c_client *client, const struct i2c_device_id *dev_id) {
    int ret = 0;
    struct piu_data *data = NULL;

    printk("addr: 0x%x\n", client->addr);

    if (client->addr != PIU_I2C_ADDR) {
        return -ENODEV;
    }

    data = kzalloc(sizeof(*data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }

    ret = sysfs_create_group(&client->dev.kobj, &piu_dev_group);
    if (ret) {
        goto err_sysfs_create;
    }

    i2c_set_clientdata(client, data);
    mutex_init(&data->lock);
    data->port = dev_id->driver_data;
    data->client = client;

    cdev_init(&data->cdev, &piu_fops);
    data->cdev.owner = THIS_MODULE;
    data->cdev.ops = &piu_fops;
    ret = cdev_add(&data->cdev, MKDEV(MAJOR(piu_devt), data->port), 1);
    if (ret) {
        goto err_cdev_add;
    }

    data->dev = device_create_with_groups(piu_sysfs_class, NULL, MKDEV(MAJOR(piu_devt), data->port), data, piu_dev_groups, "piu%d", data->port);
    if (IS_ERR(data->dev)) {
        ret = PTR_ERR(data->dev);
        goto err_dev_create;
    }

    printk("piu driver(major: %d, minor: %d) installed.\n", MAJOR(piu_devt), data->port);

    return ret;

err_dev_create:
    cdev_del(&data->cdev);
err_cdev_add:
    sysfs_remove_group(&client->dev.kobj, &piu_dev_group);
err_sysfs_create:
    kfree(data);
    return ret;
}

static int piu_device_remove(struct i2c_client *client) {
    int ret = 0;
    struct piu_data *data = i2c_get_clientdata(client);
    device_destroy(piu_sysfs_class, data->dev->devt);
    cdev_del(&data->cdev);
    sysfs_remove_group(&client->dev.kobj, &piu_dev_group);
    kfree(data);
    return ret;
}

static struct i2c_driver piu_driver = {
    .driver = {
        .name = DRIVER_NAME,
    },
    .probe    = piu_device_probe,
    .remove   = piu_device_remove,
    .id_table = piu_device_id,
};

static int __init piu_init(void)
{
    int rc;

    rc = alloc_chrdev_region(&piu_devt, 0, PIU_NUM, "piu");
    if (rc) {
        return rc;
    }

    piu_sysfs_class = class_create(THIS_MODULE, "piu");
    if (IS_ERR(piu_sysfs_class)) {
        printk("failed to create class PIU\n");
        rc = PTR_ERR(piu_sysfs_class);
        goto err_class_create;
    }

    rc = i2c_add_driver(&piu_driver);
    if (rc) {
        goto err_add_driver;
    }
    return rc;

err_add_driver:
    class_destroy(piu_sysfs_class);
err_class_create:
    unregister_chrdev_region(piu_devt, PIU_NUM);
    return rc;
}

static void __exit piu_exit(void)
{
    i2c_del_driver(&piu_driver);
    class_destroy(piu_sysfs_class);
    unregister_chrdev_region(piu_devt, PIU_NUM);
}

MODULE_AUTHOR("Wataru Ishida <ishida@nel-america.com>");
MODULE_DESCRIPTION("CFP2 transceiver PIU driver");
MODULE_LICENSE("GPL");

module_init(piu_init);
module_exit(piu_exit);
