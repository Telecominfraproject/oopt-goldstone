/*
 * Based on:
 *	pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *	pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *	i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *	pca9540.c from Jean Delvare <khali@linux-fr.org>.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/device.h>
#include <linux/hwmon-sysfs.h>
#include <linux/init.h>
#include <linux/ipmi.h>
#include <linux/ipmi_smi.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/i2c.h>

#define DRVNAME "wtp_01_02_00_sys"
#define IPMI_NETFN 0x0A
#define IPMI_SYSFRU_GET_CMD 0x11
#define IPMI_TIMEOUT (5 * HZ)
#define IPMI_READ_MAX_LEN 128

#define EEPROM_NAME "eeprom"
#define EEPROM_SIZE 256 /*	256 byte eeprom */

#define FPGA_NAME "fpga"
#define FPGA_SIZE 32 /*	32 byte memory space*/

#define SYS_FPGA_I2C_ADDR 0x30

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static int wistron_wtp_01_02_00_sys_probe(struct platform_device *pdev);
static int wistron_wtp_01_02_00_sys_remove(struct platform_device *pdev);

struct ipmi_data {
  struct completion read_complete;
  struct ipmi_addr address;
  struct ipmi_user *user;
  int interface;

  struct kernel_ipmi_msg tx_message;
  long tx_msgid;

  void *rx_msg_data;
  unsigned short rx_msg_len;
  unsigned char rx_result;
  int rx_recv_type;

  struct ipmi_user_hndl ipmi_hndlrs;
};

struct wistron_wtp_01_02_00_sys_data {
  struct platform_device *pdev;
  struct mutex update_lock;
  char valid;                 /* != 0 if registers are valid */
  unsigned long last_updated; /* In jiffies */
  unsigned char ipmi_resp[256];
  struct ipmi_data ipmi;
  struct bin_attribute eeprom; /* eeprom data */
  struct i2c_client *fpga_dev;
};

struct wistron_wtp_01_02_00_sys_data *data = NULL;

static struct platform_driver wistron_wtp_01_02_00_sys_driver = {
    .probe = wistron_wtp_01_02_00_sys_probe,
    .remove = wistron_wtp_01_02_00_sys_remove,
    .driver =
        {
            .name = DRVNAME,
            .owner = THIS_MODULE,
        },
};

struct sys_fpga {
    struct mutex lock;
    struct i2c_client *client;
};

/* Functions to talk to the IPMI layer */

/* Initialize IPMI address, message buffers and user data */
static int init_ipmi_data(struct ipmi_data *ipmi, int iface,
                          struct device *dev) {
  int err;

  init_completion(&ipmi->read_complete);

  /* Initialize IPMI address */
  ipmi->address.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
  ipmi->address.channel = IPMI_BMC_CHANNEL;
  ipmi->address.data[0] = 0;
  ipmi->interface = iface;

  /* Initialize message buffers */
  ipmi->tx_msgid = 0;
  ipmi->tx_message.netfn = IPMI_NETFN;

  ipmi->ipmi_hndlrs.ipmi_recv_hndl = ipmi_msg_handler;

  /* Create IPMI messaging interface user */
  err =
      ipmi_create_user(ipmi->interface, &ipmi->ipmi_hndlrs, ipmi, &ipmi->user);
  if (err < 0) {
    dev_err(dev,
            "Unable to register user with IPMI "
            "interface %d\n",
            ipmi->interface);
    return -EACCES;
  }

  return 0;
}

/* Send an IPMI command */
static int ipmi_send_message(struct ipmi_data *ipmi, unsigned char cmd,
                             unsigned char *tx_data, unsigned short tx_len,
                             unsigned char *rx_data, unsigned short rx_len) {
  int err;

  ipmi->tx_message.cmd = cmd;
  ipmi->tx_message.data = tx_data;
  ipmi->tx_message.data_len = tx_len;
  ipmi->rx_msg_data = rx_data;
  ipmi->rx_msg_len = rx_len;

  err = ipmi_validate_addr(&ipmi->address, sizeof(ipmi->address));
  if (err) goto addr_err;

  ipmi->tx_msgid++;
  err = ipmi_request_settime(ipmi->user, &ipmi->address, ipmi->tx_msgid,
                             &ipmi->tx_message, ipmi, 0, 0, 0);
  if (err) goto ipmi_req_err;

  err = wait_for_completion_timeout(&ipmi->read_complete, IPMI_TIMEOUT);
  if (!err) goto ipmi_timeout_err;

  return 0;

ipmi_timeout_err:
  err = -ETIMEDOUT;
  dev_err(&data->pdev->dev, "request_timeout=%x\n", err);
  return err;
ipmi_req_err:
  dev_err(&data->pdev->dev, "request_settime=%x\n", err);
  return err;
addr_err:
  dev_err(&data->pdev->dev, "validate_addr=%x\n", err);
  return err;
}

/* Dispatch IPMI messages to callers */
static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data) {
  unsigned short rx_len;
  struct ipmi_data *ipmi = user_msg_data;

  if (msg->msgid != ipmi->tx_msgid) {
    dev_err(&data->pdev->dev,
            "Mismatch between received msgid "
            "(%02x) and transmitted msgid (%02x)!\n",
            (int)msg->msgid, (int)ipmi->tx_msgid);
    ipmi_free_recv_msg(msg);
    return;
  }

  ipmi->rx_recv_type = msg->recv_type;
  if (msg->msg.data_len > 0)
    ipmi->rx_result = msg->msg.data[0];
  else
    ipmi->rx_result = IPMI_UNKNOWN_ERR_COMPLETION_CODE;

  if (msg->msg.data_len > 1) {
    rx_len = msg->msg.data_len - 1;
    if (ipmi->rx_msg_len < rx_len) rx_len = ipmi->rx_msg_len;
    ipmi->rx_msg_len = rx_len;
    memcpy(ipmi->rx_msg_data, msg->msg.data + 1, ipmi->rx_msg_len);
  } else
    ipmi->rx_msg_len = 0;

  ipmi_free_recv_msg(msg);
  complete(&ipmi->read_complete);
}

static ssize_t sys_eeprom_read(loff_t off, char *buf, size_t count) {
  int status = 0;
  unsigned char ipmi_resp[35] = {0}, ipmi_tx_data[4] = {0};

  if ((off + count) > EEPROM_SIZE) {
    return -EINVAL;
  }

  ipmi_resp[0] = '\0';
  ipmi_tx_data[0] = 0x00;                  /* FRUid */
  ipmi_tx_data[1] = (uint8_t)(off & 0xff); /* offset */
  ipmi_tx_data[2] = (uint8_t)(off >> 8);
  ipmi_tx_data[3] = 32; /* No of bytes to read*/

  status = ipmi_send_message(&data->ipmi, IPMI_SYSFRU_GET_CMD, &ipmi_tx_data[0],
                             4, &ipmi_resp[0], sizeof(ipmi_resp));

  if (unlikely(status != 0)) {
    goto exit;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto exit;
  }

  status = ipmi_resp[0];
  memcpy(buf, &ipmi_resp[1], 32);

exit:
  return status;
}

static ssize_t sysfs_bin_read(struct file *filp, struct kobject *kobj,
                              struct bin_attribute *attr, char *buf, loff_t off,
                              size_t count) {
  ssize_t retval = 0;

  if (unlikely(!count)) {
    return count;
  }

  /*
   * Read data from chip, protecting against concurrent updates
   * from this host
   */
  mutex_lock(&data->update_lock);

  while (count) {
    ssize_t status;
    status = sys_eeprom_read(off, buf, count);
    if (status <= 0) {
      if (retval == 0) {
        retval = status;
      }
      break;
    }

    buf += status;
    off += status;
    count -= status;
    retval += status;
  }

  mutex_unlock(&data->update_lock);
  return retval;
}

static int sysfs_eeprom_init(struct kobject *kobj,
                             struct bin_attribute *eeprom) {
  sysfs_bin_attr_init(eeprom);
  eeprom->attr.name = EEPROM_NAME;
  eeprom->attr.mode = S_IRUGO;
  eeprom->read = sysfs_bin_read;
  eeprom->write = NULL;
  eeprom->size = EEPROM_SIZE;

  /* Create eeprom file */
  return sysfs_create_bin_file(kobj, eeprom);
}

static int sysfs_eeprom_cleanup(struct kobject *kobj,
                                struct bin_attribute *eeprom) {
  sysfs_remove_bin_file(kobj, eeprom);
  return 0;
}

static int wistron_wtp_01_02_00_sys_probe(struct platform_device *pdev) {
  int status = -1;

  /* Register sysfs hooks */
  status = sysfs_eeprom_init(&pdev->dev.kobj, &data->eeprom);
  if (status) {
    goto exit;
  }

  dev_info(&pdev->dev, "device created\n");

  return 0;

exit:
  return status;
}

static int wistron_wtp_01_02_00_sys_remove(struct platform_device *pdev) {
  sysfs_eeprom_cleanup(&pdev->dev.kobj, &data->eeprom);

  return 0;
}

static ssize_t show_piu_reset(int index, struct device *dev, struct device_attribute *attr, char *buf) {
    int v;
    mutex_lock(&data->update_lock);
    v = i2c_smbus_read_byte_data(data->fpga_dev, 0x0e);
    mutex_unlock(&data->update_lock);
    return scnprintf(buf, PAGE_SIZE, "%d\n", (v & (1 << (index + 3))) > 0);
}

static ssize_t store_piu_reset(int index, struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    long value;

    if (kstrtoul(buf, 0, &value))
        return -EINVAL;

    /*
     * Read data from chip, protecting against concurrent updates
     * from this host
     */
    mutex_lock(&data->update_lock);

    if ( value > 0 ) {
        int v = i2c_smbus_read_byte_data(data->fpga_dev, 0x0e);
        v |= (1 << (index + 3));
        i2c_smbus_write_byte_data(data->fpga_dev, 0x0e, v);
    }

    mutex_unlock(&data->update_lock);
    return count;
}

static ssize_t show_piu1_reset(struct device *dev, struct device_attribute *attr, char *buf) { return show_piu_reset(1, dev, attr, buf); }
static ssize_t show_piu2_reset(struct device *dev, struct device_attribute *attr, char *buf) { return show_piu_reset(2, dev, attr, buf); }
static ssize_t show_piu3_reset(struct device *dev, struct device_attribute *attr, char *buf) { return show_piu_reset(3, dev, attr, buf); }
static ssize_t show_piu4_reset(struct device *dev, struct device_attribute *attr, char *buf) { return show_piu_reset(4, dev, attr, buf); }

static ssize_t store_piu1_reset(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { return store_piu_reset(1, dev, attr, buf, count); }
static ssize_t store_piu2_reset(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { return store_piu_reset(2, dev, attr, buf, count); }
static ssize_t store_piu3_reset(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { return store_piu_reset(3, dev, attr, buf, count); }
static ssize_t store_piu4_reset(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) { return store_piu_reset(4, dev, attr, buf, count); }

static DEVICE_ATTR(piu1_reset, S_IRUGO|S_IWUSR|S_IWGRP, show_piu1_reset, store_piu1_reset);
static DEVICE_ATTR(piu2_reset, S_IRUGO|S_IWUSR|S_IWGRP, show_piu2_reset, store_piu2_reset);
static DEVICE_ATTR(piu3_reset, S_IRUGO|S_IWUSR|S_IWGRP, show_piu3_reset, store_piu3_reset);
static DEVICE_ATTR(piu4_reset, S_IRUGO|S_IWUSR|S_IWGRP, show_piu4_reset, store_piu4_reset);

static struct attribute *fpga_attrs[] = {
    &dev_attr_piu1_reset.attr,
    &dev_attr_piu2_reset.attr,
    &dev_attr_piu3_reset.attr,
    &dev_attr_piu4_reset.attr,
    NULL
};

static ssize_t fpga_read(struct file *filp, struct kobject *kobj,
                         struct bin_attribute *attr, char *buf, loff_t off,
                         size_t count) {
  ssize_t retval = 0;
  int i;

  if (unlikely(!count)) {
    return count;
  }

  /*
   * Read data from chip, protecting against concurrent updates
   * from this host
   */
  mutex_lock(&data->update_lock);

  for(i = 0; i < FPGA_SIZE && count; i++) {
    buf[i] = i2c_smbus_read_byte_data(data->fpga_dev, i);
    count--;
    off++;
    retval++;
  }

  mutex_unlock(&data->update_lock);
  return retval;
}

static BIN_ATTR_RO(fpga, FPGA_SIZE);

static struct bin_attribute *fpga_bin_attrs[] = {
    &bin_attr_fpga,
    NULL
};

static struct attribute_group fpga_group = {
    .attrs = fpga_attrs,
    .bin_attrs = fpga_bin_attrs,
};

static int wtp_01_02_00_sys_fpga_device_probe(struct i2c_client *client, const struct i2c_device_id *dev_id) {
  int ret = 0;
  if (client->addr != SYS_FPGA_I2C_ADDR) {
      return -ENODEV;
  }

  ret = sysfs_create_group(&client->dev.kobj, &fpga_group);
  if (ret) return ret;
  data->fpga_dev = client;
  return 0;
}

static int wtp_01_02_00_sys_fpga_device_remove(struct i2c_client *client) {
  sysfs_remove_group(&client->dev.kobj, &fpga_group);
  return 0;
}

static const struct i2c_device_id wtp_01_02_00_sys_fpga_device_id[] = {
  { "sys_fpga", 0, },
  { /* end of list */ },
};

static struct i2c_driver wtp_01_02_00_sys_fpga_driver = {
    .driver = {
        .name = "wtp_01_02_00_sys_fpga",
    },
    .probe    = wtp_01_02_00_sys_fpga_device_probe,
    .remove   = wtp_01_02_00_sys_fpga_device_remove,
    .id_table = wtp_01_02_00_sys_fpga_device_id,
};

static int __init wistron_wtp_01_02_00_sys_init(void) {
  int ret;

  data = kzalloc(sizeof(struct wistron_wtp_01_02_00_sys_data), GFP_KERNEL);
  if (!data) {
    ret = -ENOMEM;
    goto alloc_err;
  }

  mutex_init(&data->update_lock);
  data->valid = 0;

  ret = platform_driver_register(&wistron_wtp_01_02_00_sys_driver);
  if (ret < 0) {
    goto dri_reg_err;
  }

  data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
  if (IS_ERR(data->pdev)) {
    ret = PTR_ERR(data->pdev);
    goto dev_reg_err;
  }

  /* Set up IPMI interface */
  ret = init_ipmi_data(&data->ipmi, 0, &data->pdev->dev);
  if (ret) goto ipmi_err;

  ret = i2c_add_driver(&wtp_01_02_00_sys_fpga_driver);
  if (ret) goto i2c_err;

  return 0;

i2c_err:
  ipmi_destroy_user(data->ipmi.user);
ipmi_err:
  platform_device_unregister(data->pdev);
dev_reg_err:
  platform_driver_unregister(&wistron_wtp_01_02_00_sys_driver);
dri_reg_err:
  kfree(data);
alloc_err:
  return ret;
}

static void __exit wistron_wtp_01_02_00_sys_exit(void) {
  i2c_del_driver(&wtp_01_02_00_sys_fpga_driver);
  ipmi_destroy_user(data->ipmi.user);
  platform_device_unregister(data->pdev);
  platform_driver_unregister(&wistron_wtp_01_02_00_sys_driver);
  kfree(data);
}

MODULE_AUTHOR("HarshaF1");
MODULE_DESCRIPTION("WISTRON-WTP_01_02_00 SYS driver");
MODULE_LICENSE("GPL");

module_init(wistron_wtp_01_02_00_sys_init);
module_exit(wistron_wtp_01_02_00_sys_exit);
