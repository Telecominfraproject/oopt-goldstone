#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/hwmon-sysfs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#include "piu.h"
#include "aco.h"
#include "dco.h"

#define DRIVER_NAME "piu"

static struct class *piu_sysfs_class = NULL;
static dev_t piu_devt;

#define PIU_I2C_ADDR 0x6A
#define PIU_NUM 8

#define CFP2_MODULE_TEMP    0xb02f
#define CFP2_TX_LASER_TEMP  0xb340
#define CFP2_RX_LASER_TEMP  0xb380
#define CFP2_VENDOR_NAME    0x8021
#define CFP2_PART_NUMBER    0x8034
#define CFP2_SERIAL_NUMBER  0x8044

enum piu_type {
    PIU_TYPE_UNKNOWN,
    PIU_TYPE_UNPLUGGED,
    PIU_TYPE_ACO,
    PIU_TYPE_DCO,
    PIU_TYPE_QSFP28
};

struct piu_dev {
    struct mutex lock;
    struct i2c_client *client;
    struct i2c_client *dco_client;
    int port;
    struct cdev cdev;
    struct device *dev;
    struct bin_attribute eeprom[2];
    enum piu_type type;
    bool simulate_plug_out;
    bool simulate_cfp2_plug_out;
};

static int piu_i2c_read(struct piu_dev *dev, uint32_t addr, uint32_t *value);

static enum piu_type get_piu_type(struct piu_dev *piu, int *value) {
    enum piu_type ret;
    int type;
    mutex_lock(&piu->lock);

    if (piu->simulate_plug_out) {
        ret = PIU_TYPE_UNPLUGGED;
        goto out;
    }

    type = i2c_smbus_read_byte_data(piu->client, 0);
    if ( type >= 0 ) {
        if (value) *value = type;
        if ( type == 0x10 || type == 0x11 || type == 0x12 ) {
            ret = PIU_TYPE_DCO;
        } else if ( type == 0xff ) {
            ret = PIU_TYPE_ACO;
        } else if ( type == 0x00 || type == 0x01 || type == 0x02 ) {
            ret = PIU_TYPE_QSFP28;
        } else {
            ret = PIU_TYPE_UNKNOWN;
        }
    } else {
        if ( dco_piu_i2c_read(piu->dco_client, 1, &type) < 0 ) {
            ret = PIU_TYPE_UNPLUGGED;
        } else if ( type == 0x0 ) {
            ret = PIU_TYPE_DCO;
        } else if ( type == 0x1 ) {
            ret = PIU_TYPE_QSFP28;
        } else {
            ret = PIU_TYPE_UNKNOWN;
        }
        if (value) *value = type;
    }
out:
    piu->type = ret;
    mutex_unlock(&piu->lock);
    return ret;
}

static int cfp2_exists(struct piu_dev *piu, enum piu_type type) {
    int ret;
    mutex_lock(&piu->lock);

    if (piu->simulate_plug_out || piu->simulate_cfp2_plug_out) {
        ret = 0;
        goto out;
    }

    switch (type) {
    case PIU_TYPE_ACO:
        ret = aco_cfp2_exists(piu->client);
        break;
    case PIU_TYPE_DCO:
        ret = dco_cfp2_exists(piu->dco_client);
        break;
    default:
        ret = 1;
    }
out:
    mutex_unlock(&piu->lock);
    return ret;
}

static ssize_t piu_index_show(struct device *dev, struct device_attribute *da, char *buf) {
    struct piu_dev *priv = dev_get_drvdata(dev);
    return scnprintf(buf, PAGE_SIZE, "%d\n", priv->port);
}
static DEVICE_ATTR_RO(piu_index);

static ssize_t piu_type_show(struct device *dev, struct device_attribute *da, char *buf) {
    int value;
    enum piu_type type;
    struct piu_dev *priv = dev_get_drvdata(dev);
    type = get_piu_type(priv, &value);
    switch (type) {
    case PIU_TYPE_UNPLUGGED:
        return scnprintf(buf, PAGE_SIZE, "\n");
    case PIU_TYPE_ACO:
        return scnprintf(buf, PAGE_SIZE, "ACO\n");
    case PIU_TYPE_DCO:
        return scnprintf(buf, PAGE_SIZE, "DCO\n");
    case PIU_TYPE_QSFP28:
        return scnprintf(buf, PAGE_SIZE, "QSFP28\n");
    default:
        return scnprintf(buf, PAGE_SIZE, "UNKNOWN(0x%x)\n", value);
    }
}
static DEVICE_ATTR_RO(piu_type);

static ssize_t piu_mcu_version_show(struct device *dev, struct device_attribute *da, char *buf) {
    int value;
    enum piu_type type;
    struct piu_dev *priv = dev_get_drvdata(dev);
    uint32_t version[2];
    type = get_piu_type(priv, &value);
    switch (type) {
    case PIU_TYPE_DCO:
    case PIU_TYPE_QSFP28:
        piu_i2c_read(priv, 2, &version[0]);
        piu_i2c_read(priv, 3, &version[1]);
        return scnprintf(buf, PAGE_SIZE, "%u.%u.%u\n", (version[0] >> 4) & 15, version[0] & 15, (version[1] >> 4) & 15);
    case PIU_TYPE_ACO:
        piu_i2c_read(priv, ACO_PIU_FPGA_VERSION, &version[0]);
        return scnprintf(buf, PAGE_SIZE, "%u.%u.%u\n", (version[0] >> 24) & 15, (version[0] >> 20) & 15, (version[0] >> 16) & 15);
    default:
        return scnprintf(buf, PAGE_SIZE, "\n");
    }
}
static DEVICE_ATTR_RO(piu_mcu_version);

static ssize_t piu_temp_show(struct device *dev, struct device_attribute *da, char *buf) {
    int value;
    enum piu_type type;
    struct piu_dev *priv = dev_get_drvdata(dev);
    type = get_piu_type(priv, &value);
    switch (type) {
    case PIU_TYPE_ACO:
        {
            uint32_t thermal = 0;
            piu_i2c_read(priv, ACO_PIU_THERMAL_ENABLED, &thermal);
            if ( thermal != 1 ) {
                return scnprintf(buf, PAGE_SIZE, "\n");
            }
            thermal = 0;
            piu_i2c_read(priv, ACO_PIU_THERMAL_VALUE, &thermal);
            return scnprintf(buf, PAGE_SIZE, "%d\n", thermal);
        }
    default:
        return scnprintf(buf, PAGE_SIZE, "\n");
    }
}

static DEVICE_ATTR_RO(piu_temp);

static ssize_t cfp2_temp_show(struct device *dev, uint32_t addr, struct device_attribute *da, char *buf) {
    int value;
    enum piu_type type;
    struct piu_dev *priv = dev_get_drvdata(dev);
    type = get_piu_type(priv, &value);
    switch (type) {
    case PIU_TYPE_ACO:
    case PIU_TYPE_DCO:
        if ( cfp2_exists(priv, type) ) {
            uint32_t offset = type == PIU_TYPE_ACO ? ACO_PIU_CFP2_OFFSET : DCO_PIU_CFP2_OFFSET;
            int32_t temp = 0;
            piu_i2c_read(priv, offset | addr, (uint32_t*)&temp);
            return scnprintf(buf, PAGE_SIZE, "%d.%d\n", temp/256, temp%256);
        }
        return scnprintf(buf, PAGE_SIZE, "\n");
    default:
        return scnprintf(buf, PAGE_SIZE, "\n");
    }
}

static ssize_t cfp2_exists_show(struct device *dev, struct device_attribute *da, char *buf) {
    int value;
    enum piu_type type;
    struct piu_dev *priv = dev_get_drvdata(dev);
    type = get_piu_type(priv, &value);
    return scnprintf(buf, PAGE_SIZE, "%d\n", cfp2_exists(priv, type));
}
static DEVICE_ATTR_RO(cfp2_exists);

static ssize_t cfp2_cage_temp_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_temp_show(dev, CFP2_MODULE_TEMP, da, buf);
}
static DEVICE_ATTR_RO(cfp2_cage_temp);

static ssize_t cfp2_tx_laser_temp_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_temp_show(dev, CFP2_TX_LASER_TEMP, da, buf);
}
static DEVICE_ATTR_RO(cfp2_tx_laser_temp);

static ssize_t cfp2_rx_laser_temp_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_temp_show(dev, CFP2_RX_LASER_TEMP, da, buf);
}
static DEVICE_ATTR_RO(cfp2_rx_laser_temp);

static ssize_t cfp2_string_show(struct device *dev, uint32_t addr, ssize_t size, struct device_attribute *da, char *buf) {
    int value;
    enum piu_type type;
    struct piu_dev *priv = dev_get_drvdata(dev);
    type = get_piu_type(priv, &value);
    switch (type) {
    case PIU_TYPE_ACO:
    case PIU_TYPE_DCO:
        if ( cfp2_exists(priv, type) ) {
            int i;
            uint32_t offset = type == PIU_TYPE_ACO ? ACO_PIU_CFP2_OFFSET : DCO_PIU_CFP2_OFFSET;
            for ( i = 0; i < size; i++ ) {
                piu_i2c_read(priv, (offset | addr) + i, (uint32_t*)&buf[i]);
            }
            while ( isspace(buf[i]) || buf[i] == 0 ) {
                buf[i--] = 0;
            }
            buf[i+1] = '\n';
            return i+2;
        }
    default:
        return scnprintf(buf, PAGE_SIZE, "\n");
    }
}

static ssize_t cfp2_vendor_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_string_show(dev, CFP2_VENDOR_NAME, 16, da, buf);
}
static DEVICE_ATTR_RO(cfp2_vendor);

static ssize_t cfp2_part_number_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_string_show(dev, CFP2_PART_NUMBER, 16, da, buf);
}
static DEVICE_ATTR_RO(cfp2_part_number);

static ssize_t cfp2_serial_number_show(struct device *dev, struct device_attribute *da, char *buf) {
    return cfp2_string_show(dev, CFP2_SERIAL_NUMBER, 16, da, buf);
}
static DEVICE_ATTR_RO(cfp2_serial_number);

static ssize_t cfp2_oui_show(struct device *dev, struct device_attribute *da, char *buf) {
    int value;
    enum piu_type type;
    struct piu_dev *priv = dev_get_drvdata(dev);
    type = get_piu_type(priv, &value);
    switch (type) {
    case PIU_TYPE_ACO:
    case PIU_TYPE_DCO:
        if ( cfp2_exists(priv, type) ) {
            uint32_t offset = type == PIU_TYPE_ACO ? ACO_PIU_CFP2_OFFSET : DCO_PIU_CFP2_OFFSET;
            uint32_t oui, data;
            piu_i2c_read(priv, offset | 0x8031, (uint32_t*)&data);
            oui = (data &= 0xff) << 8;
            piu_i2c_read(priv, offset | 0x8032, (uint32_t*)&data);
            oui = (oui | (data &= 0xff)) << 8;
            piu_i2c_read(priv, offset | 0x8033, (uint32_t*)&data);
            oui = (oui | (data &= 0xff));
            return scnprintf(buf, PAGE_SIZE, "%d\n", oui);
        }
    default:
        return scnprintf(buf, PAGE_SIZE, "\n");
    }
}
static DEVICE_ATTR_RO(cfp2_oui);

static ssize_t cfp2_firmware_version_show(struct device *dev, struct device_attribute *da, char *buf) {
    int value;
    enum piu_type type;
    struct piu_dev *priv = dev_get_drvdata(dev);
    type = get_piu_type(priv, &value);
    switch (type) {
    case PIU_TYPE_ACO:
    case PIU_TYPE_DCO:
        if ( cfp2_exists(priv, type) ) {
            uint32_t offset = type == PIU_TYPE_ACO ? ACO_PIU_CFP2_OFFSET : DCO_PIU_CFP2_OFFSET;
            uint32_t data[2];
            piu_i2c_read(priv, offset | 0x806C, &data[0]);
            piu_i2c_read(priv, offset | 0x806D, &data[1]);
            return scnprintf(buf, PAGE_SIZE, "%d.%d\n", data[0] & 0xff, data[1] & 0xff);
        }
    default:
        return scnprintf(buf, PAGE_SIZE, "\n");
    }
}
static DEVICE_ATTR_RO(cfp2_firmware_version);

static ssize_t piu_simulate_plug_out_show(struct device *dev, struct device_attribute *da, char *buf) {
    struct piu_dev *priv = dev_get_drvdata(dev);
    if ( priv != NULL && priv->simulate_plug_out ) {
        return scnprintf(buf, PAGE_SIZE, "1");
    }
    return scnprintf(buf, PAGE_SIZE, "0");
}

static ssize_t piu_simulate_plug_out_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count) {
    struct piu_dev *priv = dev_get_drvdata(dev);
    long value;

    if (kstrtoul(buf, 0, &value))
        return -EINVAL;

    mutex_lock(&priv->lock);
    priv->simulate_plug_out = value > 0;
    mutex_unlock(&priv->lock);
    return count;
}
static DEVICE_ATTR_RW(piu_simulate_plug_out);

static ssize_t cfp2_simulate_plug_out_show(struct device *dev, struct device_attribute *da, char *buf) {
    struct piu_dev *priv = dev_get_drvdata(dev);
    if ( priv != NULL && priv->simulate_cfp2_plug_out ) {
        return scnprintf(buf, PAGE_SIZE, "1");
    }
    return scnprintf(buf, PAGE_SIZE, "0");
}

static ssize_t cfp2_simulate_plug_out_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count) {
    struct piu_dev *priv = dev_get_drvdata(dev);
    long value;

    if (kstrtoul(buf, 0, &value))
        return -EINVAL;

    mutex_lock(&priv->lock);
    priv->simulate_cfp2_plug_out = value > 0;
    mutex_unlock(&priv->lock);
    return count;
}
static DEVICE_ATTR_RW(cfp2_simulate_plug_out);

static struct attribute *piu_dev_attrs[] = {
    &dev_attr_piu_index.attr,
    &dev_attr_piu_type.attr,
    &dev_attr_piu_mcu_version.attr,
    &dev_attr_piu_temp.attr,
    &dev_attr_piu_simulate_plug_out.attr,
    &dev_attr_cfp2_exists.attr,
    &dev_attr_cfp2_cage_temp.attr,
    &dev_attr_cfp2_tx_laser_temp.attr,
    &dev_attr_cfp2_rx_laser_temp.attr,
    &dev_attr_cfp2_vendor.attr,
    &dev_attr_cfp2_part_number.attr,
    &dev_attr_cfp2_serial_number.attr,
    &dev_attr_cfp2_oui.attr,
    &dev_attr_cfp2_firmware_version.attr,
    &dev_attr_cfp2_simulate_plug_out.attr,
    NULL
};
ATTRIBUTE_GROUPS(piu_dev);

static int piu_i2c_read(struct piu_dev *dev, uint32_t addr, uint32_t *value) {
    int ret;
    mutex_lock(&dev->lock);

    if (dev->simulate_plug_out) {
        ret = -1;
        goto out;
    }

    switch (dev->type) {
    case PIU_TYPE_ACO:
        if (dev->simulate_cfp2_plug_out && (addr & ACO_PIU_CFP2_OFFSET)) {
            ret = -1;
            goto out;
        }
        ret = aco_piu_i2c_read(dev->client, addr, value);
        break;
    case PIU_TYPE_DCO:
        if (dev->simulate_cfp2_plug_out && (addr & DCO_PIU_CFP2_OFFSET)) {
            ret = -1;
            goto out;
        }
    case PIU_TYPE_QSFP28:
        ret = dco_piu_i2c_read(dev->dco_client, addr, value);
        break;
    default:
        printk("read for type 0x%x is not supported\n", dev->type);
        ret = -1;
    }

out:
    mutex_unlock(&dev->lock);
    return ret;
}

static int piu_i2c_write(struct piu_dev *dev, uint32_t addr, uint32_t value) {
    int ret;
    mutex_lock(&dev->lock);

    if (dev->simulate_plug_out) {
        ret = -1;
        goto out;
    }

    switch (dev->type) {
    case PIU_TYPE_ACO:
        if (dev->simulate_cfp2_plug_out && (addr & ACO_PIU_CFP2_OFFSET)) {
            ret = -1;
            goto out;
        }
        ret = aco_piu_i2c_write(dev->client, addr, value);
        break;
    case PIU_TYPE_DCO:
        if (dev->simulate_cfp2_plug_out && (addr & DCO_PIU_CFP2_OFFSET)) {
            ret = -1;
            goto out;
        }
    case PIU_TYPE_QSFP28:
        ret = dco_piu_i2c_write(dev->dco_client, addr, value);
        break;
    default:
        printk("write for type 0x%x is not supported\n", dev->type);
        ret = -1;
    }

out:
    mutex_unlock(&dev->lock);
    return ret;
}

long piu_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    struct piu_dev *dev = filp->private_data;
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

    switch (cmd) {
        case PIU_READ:
            if (piu_i2c_read(dev, data.reg, &data.val)) {
                retval = -EFAULT;
                goto done;
            }
            if ( copy_to_user((int __user *)arg, &data, sizeof(data)) ) {
                retval = -EFAULT;
                goto done;
            }
            break;
        case PIU_WRITE:
            if (piu_i2c_write(dev, data.reg, data.val)) {
                retval = -EFAULT;
                goto done;
            }
            break;
        case PIU_INIT:
            // update the piu type
            get_piu_type(dev, NULL);
            break;
        default:
            retval = -ENOTTY;
            break;
    }

done:
    return retval;
}

int piu_close(struct inode *inode, struct file *filp) {
    printk("close\n");
    return 0;
}

int piu_open(struct inode *inode, struct file *filp) {
    struct piu_dev *priv = container_of(inode->i_cdev, struct piu_dev, cdev);
    enum piu_type type = get_piu_type(priv, NULL);
    printk("open: piu-type: 0x%x\n", type);
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

static ssize_t eeprom_bin_read(struct file *filp, struct kobject *kobj,
                                   struct bin_attribute *attr, char *buf, loff_t off,
                                   size_t count) {
    struct device *dev = container_of(kobj, struct device, kobj);
    struct piu_dev *piu = dev->driver_data;
    int value, i;
    uint32_t offset;
    enum piu_type type;
    type = get_piu_type(piu, &value);
    offset = &piu->eeprom[0] == attr ? QSFP28_1_DEV_ADDR_OFFSET : QSFP28_2_DEV_ADDR_OFFSET;
    if ( type != PIU_TYPE_QSFP28 ) {
        return -EIO;
    }

    for ( i = off; i < off + count; i++ ) {
        uint32_t v;
        if ( piu_i2c_read(piu, offset | i, &v) < 0 ) {
            return -EIO;
        }
        buf[i] = v;
    }

    return count;
}

/* Platform dependent +++ */
static int piu_device_probe(struct i2c_client *client, const struct i2c_device_id *dev_id) {
    int ret = 0;
    struct piu_dev *data = NULL;

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
    data->dco_client = i2c_new_dummy_device(client->adapter, client->addr + 1);
    data->simulate_plug_out = false;
    data->simulate_cfp2_plug_out = false;

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

    {
        char buf[32];
        int i;

        for ( i = 0; i < 2; i++ ) {
            sysfs_bin_attr_init(data->eeprom[i]);
            scnprintf(buf, 32, "qsfp28_%d_eeprom", i+1);
            data->eeprom[i].attr.name = buf;
            data->eeprom[i].attr.mode = S_IRUGO;
            data->eeprom[i].read = eeprom_bin_read;
            data->eeprom[i].size = QSFP28_EEPROM_SIZE;
            sysfs_create_bin_file(&data->dev->kobj, &data->eeprom[i]);
        }
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
    struct piu_dev *data = i2c_get_clientdata(client);
    device_destroy(piu_sysfs_class, data->dev->devt);
    cdev_del(&data->cdev);
    sysfs_remove_group(&client->dev.kobj, &piu_dev_group);
    i2c_unregister_device(data->dco_client);
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
