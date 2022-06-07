#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/hwmon-sysfs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include "freescale/enetc/enetc.h"

#include <linux/ioctl.h>

struct cfp2dco_cmd {
    unsigned int reg;
    unsigned int offset;
    unsigned int val;
};

#define IOC_MAGIC 'p'

#define CFP2DCO_READ  _IOWR(IOC_MAGIC, 1, struct cfp2dco_cmd)
#define CFP2DCO_WRITE _IOWR(IOC_MAGIC, 2, struct cfp2dco_cmd)

static struct class *cfp2dco_sysfs_class = NULL;
static dev_t cfp2dco_devt;

#define CFP2DCO_NUM 4

#define CFP2_MODULE_TEMP    0xb02f
#define CFP2_TX_LASER_TEMP  0xb340
#define CFP2_RX_LASER_TEMP  0xb380
#define CFP2_VENDOR_NAME    0x8021
#define CFP2_PART_NUMBER    0x8034
#define CFP2_SERIAL_NUMBER  0x8044

struct cfp2dco_dev {
    int port;
    bool simulate_plug_out;
    struct cdev cdev;
    struct device *dev;
};

static struct cfp2dco_dev cfp2dco_devs[CFP2DCO_NUM];

static int cfp2dco_read(struct cfp2dco_dev *dev, uint32_t addr, uint32_t *value) {
    int regnum = MII_ADDR_C45 | (1 << 16) | addr;

    if (dev->simulate_plug_out) {
        return -1;
    }
    mutex_lock(&mdio_demo->mdio_lock);
    *value = mdio_demo->read(mdio_demo, dev->port-1, regnum);
    mutex_unlock(&mdio_demo->mdio_lock);
    return 0;
}

static int cfp2dco_write(struct cfp2dco_dev *dev, uint32_t addr, uint32_t value) {
    int regnum = MII_ADDR_C45 | (1 << 16) | addr;
    int ret;

    if (dev->simulate_plug_out) {
        return -1;
    }
    mutex_lock(&mdio_demo->mdio_lock);
    ret = mdio_demo->write(mdio_demo, dev->port-1, regnum, value);
    mutex_unlock(&mdio_demo->mdio_lock);
    return ret;
}

static ssize_t index_show(struct device *dev, struct device_attribute *da, char *buf) {
    struct cfp2dco_dev *priv = dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%d\n", priv->port);
}
static DEVICE_ATTR_RO(index);

static ssize_t cfp2_string_show(struct device *dev, uint32_t addr, ssize_t size, struct device_attribute *da, char *buf) {
    struct cfp2dco_dev *priv = dev_get_drvdata(dev);
    int i;
    for ( i = 0; i < size; i++ ) {
        cfp2dco_read(priv, addr + i, (uint32_t*)&buf[i]);
    }
    while (i >= 0 && (isspace(buf[i]) || buf[i] == 0 || buf[i] == 0xff)) {
        buf[i--] = 0;
    }
    if (i < 0) {
        return 0;
    }
    buf[i+1] = '\n';
    return i+2;
}

static ssize_t vendor_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_string_show(dev, CFP2_VENDOR_NAME, 16, da, buf);
}
static DEVICE_ATTR_RO(vendor);

static ssize_t part_number_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_string_show(dev, CFP2_PART_NUMBER, 16, da, buf);
}
static DEVICE_ATTR_RO(part_number);

static ssize_t serial_number_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_string_show(dev, CFP2_SERIAL_NUMBER, 16, da, buf);
}
static DEVICE_ATTR_RO(serial_number);

static ssize_t oui_show(struct device *dev, struct device_attribute *da, char *buf) {
    struct cfp2dco_dev *priv = dev_get_drvdata(dev);
    uint32_t oui, data;
    cfp2dco_read(priv, 0x8031, (uint32_t*)&data);
    oui = (data &= 0xff) << 8;
    cfp2dco_read(priv, 0x8032, (uint32_t*)&data);
    oui = (oui | (data &= 0xff)) << 8;
    cfp2dco_read(priv, 0x8033, (uint32_t*)&data);
    oui = (oui | (data &= 0xff));
    return scnprintf(buf, PAGE_SIZE, "%d\n", oui);
}
static DEVICE_ATTR_RO(oui);

static ssize_t firmware_version_show(struct device *dev, struct device_attribute *da, char *buf) {
    struct cfp2dco_dev *priv = dev_get_drvdata(dev);
    uint32_t data[2];
    cfp2dco_read(priv, 0x806C, &data[0]);
    cfp2dco_read(priv, 0x806D, &data[1]);
    return scnprintf(buf, PAGE_SIZE, "%d.%d\n", data[0] & 0xff, data[1] & 0xff);
}
static DEVICE_ATTR_RO(firmware_version);

static ssize_t cfp2_temp_show(struct device *dev, uint32_t addr, struct device_attribute *da, char *buf) {
    struct cfp2dco_dev *priv = dev_get_drvdata(dev);
    int32_t temp = 0;
    cfp2dco_read(priv, addr, (uint32_t*)&temp);
    return scnprintf(buf, PAGE_SIZE, "%d.%d\n", temp/256, temp%256);
}

static ssize_t cage_temp_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_temp_show(dev, CFP2_MODULE_TEMP, da, buf);
}
static DEVICE_ATTR_RO(cage_temp);

static ssize_t tx_laser_temp_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_temp_show(dev, CFP2_TX_LASER_TEMP, da, buf);
}
static DEVICE_ATTR_RO(tx_laser_temp);

static ssize_t rx_laser_temp_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_temp_show(dev, CFP2_RX_LASER_TEMP, da, buf);
}
static DEVICE_ATTR_RO(rx_laser_temp);

static ssize_t simulate_plug_out_show(struct device *dev, struct device_attribute *da, char *buf) {
    struct cfp2dco_dev *priv = dev_get_drvdata(dev);
    if ( priv != NULL && priv->simulate_plug_out ) {
        return scnprintf(buf, PAGE_SIZE, "1");
    }
    return scnprintf(buf, PAGE_SIZE, "0");
}

static ssize_t simulate_plug_out_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count) {
    struct cfp2dco_dev *priv = dev_get_drvdata(dev);
    long value;

    if (kstrtoul(buf, 0, &value))
        return -EINVAL;

    priv->simulate_plug_out = value > 0;
    return count;
}
static DEVICE_ATTR_RW(simulate_plug_out);

static struct attribute *cfp2dco_dev_attrs[] = {
    &dev_attr_index.attr,
    &dev_attr_vendor.attr,
    &dev_attr_part_number.attr,
    &dev_attr_serial_number.attr,
    &dev_attr_oui.attr,
    &dev_attr_firmware_version.attr,
    &dev_attr_cage_temp.attr,
    &dev_attr_tx_laser_temp.attr,
    &dev_attr_rx_laser_temp.attr,
    &dev_attr_simulate_plug_out.attr,
    NULL
};

ATTRIBUTE_GROUPS(cfp2dco_dev);

long cfp2dco_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    struct cfp2dco_dev *dev = filp->private_data;
    long retval = 0;
    struct cfp2dco_cmd data;

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

    switch (cmd) {
        case CFP2DCO_READ:
            if (cfp2dco_read(dev, data.reg, &data.val)) {
                retval = -EFAULT;
                goto done;
            }
            if ( copy_to_user((int __user *)arg, &data, sizeof(data)) ) {
                retval = -EFAULT;
                goto done;
            }
            break;
        case CFP2DCO_WRITE:
            if (cfp2dco_write(dev, data.reg, data.val)) {
                retval = -EFAULT;
                goto done;
            }
            break;
        default:
            retval = -ENOTTY;
            break;
    }

done:
    return retval;
}

int cfp2dco_close(struct inode *inode, struct file *filp) {
    printk("cfp2dco close\n");
    return 0;
}

int cfp2dco_open(struct inode *inode, struct file *filp) {
    struct cfp2dco_dev *priv = container_of(inode->i_cdev, struct cfp2dco_dev, cdev);
    printk("cfp2dco open: port %d\n", priv->port);
    filp->private_data = priv;
    return 0;
}

struct file_operations cfp2dco_fops = {
    .owner = THIS_MODULE,
    .open = cfp2dco_open,
    .release = cfp2dco_close,
    .unlocked_ioctl = cfp2dco_ioctl,
};

static int __init cfp2dco_init(void)
{
    int i;
    int rc = alloc_chrdev_region(&cfp2dco_devt, 0, CFP2DCO_NUM, "cfp2dco");
    if (rc) {
        return rc;
    }

    cfp2dco_sysfs_class = class_create(THIS_MODULE, "cfp2dco");
    if (IS_ERR(cfp2dco_sysfs_class)) {
        printk("failed to create class CFP2DCO\n");
        rc = PTR_ERR(cfp2dco_sysfs_class);
        goto err_class_create;
    }

    for (i = 0; i < CFP2DCO_NUM; i++) {
        struct cfp2dco_dev *dev = &cfp2dco_devs[i];
        dev->simulate_plug_out = false;
        cdev_init(&dev->cdev, &cfp2dco_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &cfp2dco_fops;
        dev->port = i+1;

        rc = cdev_add(&dev->cdev, MKDEV(MAJOR(cfp2dco_devt), i), 1);
        if (rc) {
            goto err_cdev_add;
        }

        dev->dev = device_create_with_groups(cfp2dco_sysfs_class, NULL, MKDEV(MAJOR(cfp2dco_devt), i), dev, cfp2dco_dev_groups, "cfp2dco%d", dev->port);
        if (IS_ERR(dev->dev)) {
            rc = PTR_ERR(dev->dev);
            goto err_dev_create;
        }
        printk("cfp2dco driver(major: %d, minor: %d) installed.\n", MAJOR(cfp2dco_devt), dev->port);
    }

    return rc;

err_dev_create:
    cdev_del(&cfp2dco_devs[i].cdev);
err_cdev_add:
    i--;
    for (; i >= 0; i--) {
        device_destroy(cfp2dco_sysfs_class, cfp2dco_devs[i].dev->devt);
        cdev_del(&cfp2dco_devs[i].cdev);
    }
err_class_create:
    unregister_chrdev_region(cfp2dco_devt, CFP2DCO_NUM);
    return rc;
}

static void __exit cfp2dco_exit(void)
{
    int i;
    for (i = 0; i < CFP2DCO_NUM; i++) {
        struct cfp2dco_dev *dev = &cfp2dco_devs[i];
        printk("cfp2dco deleting port: %d\n", dev->port);
        device_destroy(cfp2dco_sysfs_class, dev->dev->devt);
        cdev_del(&dev->cdev);
    }
    class_destroy(cfp2dco_sysfs_class);
    unregister_chrdev_region(cfp2dco_devt, CFP2DCO_NUM);
}

MODULE_AUTHOR("Wataru Ishida <ishida@nel-america.com>");
MODULE_DESCRIPTION("Galileo FlexT CFP2DCO transceiver driver");
MODULE_LICENSE("GPL");

module_init(cfp2dco_init);
module_exit(cfp2dco_exit);
