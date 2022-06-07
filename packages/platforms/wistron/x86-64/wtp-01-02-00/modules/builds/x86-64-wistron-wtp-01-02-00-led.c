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

#define DRVNAME "wtp_01_02_00_led"
#define WISTRON_OEM_IPMI_NETFN 0x30
#define WISTRON_IPMI_RW_CMD 0x2b
#define IPMI_TIMEOUT (5 * HZ)

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static ssize_t set_led(struct device *dev, struct device_attribute *da,
                       const char *buf, size_t count);
static ssize_t show_led(struct device *dev, struct device_attribute *attr,
                        char *buf);
static int wistron_wtp_01_02_00_led_probe(struct platform_device *pdev);
static int wistron_wtp_01_02_00_led_remove(struct platform_device *pdev);

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

struct ipmi_led_data {
  unsigned char led_ctrl1;
  unsigned char led_ctrl2;
};

struct wistron_wtp_01_02_00_led_data {
  struct platform_device *pdev;
  struct mutex update_lock;
  char valid;                 /* != 0 if registers are valid */
  unsigned long last_updated; /* In jiffies */
  struct ipmi_led_data led_data;
  struct ipmi_data ipmi;
};

struct wistron_wtp_01_02_00_led_data *data = NULL;

static struct platform_driver wistron_wtp_01_02_00_led_driver = {
    .probe = wistron_wtp_01_02_00_led_probe,
    .remove = wistron_wtp_01_02_00_led_remove,
    .driver =
        {
            .name = DRVNAME,
            .owner = THIS_MODULE,
        },
};

/* Note: Keep the enum values same as the ones defined in
 * onlp_led_mode_t (packages/base/any/onlp/src/onlp/module/inc/onlp/led.h)
 * */
enum led_light_mode {
  LED_MODE_OFF,
  LED_MODE_RED = 10,
  LED_MODE_RED_BLINKING = 11,
  LED_MODE_ORANGE_BLINKING = 13,
  LED_MODE_YELLOW = 14,
  LED_MODE_YELLOW_BLINKING = 15,
  LED_MODE_GREEN = 16,
  LED_MODE_GREEN_BLINKING = 17,
  LED_MODE_BLUE = 18,
  LED_MODE_BLUE_BLINKING = 19,
  LED_MODE_PURPLE = 20,
  LED_MODE_PURPLE_BLINKING = 21,
  LED_MODE_AUTO = 22,
  LED_MODE_AUTO_BLINKING = 23,
  LED_MODE_WHITE = 24,
  LED_MODE_WHITE_BLINKING = 25,
  LED_MODE_CYAN = 26,
  LED_MODE_CYAN_BLINKING = 27,
  LED_MODE_UNKNOWN = 99
};

enum wistron_wtp_01_02_00_led_sysfs_attrs {
  LED_SYS,
  LED_BMC,
  LED_FAN,
  LED_PSU
};

static SENSOR_DEVICE_ATTR(led_sys, S_IWUSR | S_IRUGO, show_led, set_led,
                          LED_SYS);
static SENSOR_DEVICE_ATTR(led_bmc, S_IWUSR | S_IRUGO, show_led, set_led,
                          LED_BMC);
static SENSOR_DEVICE_ATTR(led_fan, S_IWUSR | S_IRUGO, show_led, set_led,
                          LED_FAN);
static SENSOR_DEVICE_ATTR(led_psu, S_IWUSR | S_IRUGO, show_led, set_led,
                          LED_PSU);

static struct attribute *wistron_wtp_01_02_00_led_attributes[] = {
    &sensor_dev_attr_led_sys.dev_attr.attr,
    &sensor_dev_attr_led_bmc.dev_attr.attr,
    &sensor_dev_attr_led_fan.dev_attr.attr,
    &sensor_dev_attr_led_psu.dev_attr.attr, NULL};

static const struct attribute_group wistron_wtp_01_02_00_led_group = {
    .attrs = wistron_wtp_01_02_00_led_attributes,
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
  ipmi->tx_message.netfn = WISTRON_OEM_IPMI_NETFN;

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

static struct wistron_wtp_01_02_00_led_data *
wistron_wtp_01_02_00_led_update_device(void) {
  int status = 0;
  unsigned char ipmi_resp[4] = {0}, ipmi_tx_data[4] = {0}, sys1_led, sys2_led;

  if (time_before(jiffies, data->last_updated + HZ * 5) && data->valid) {
    return data;
  }

  data->valid = 0;

  /* Get sys1_led data from ipmi */
  ipmi_resp[0] = '\0';
  ipmi_tx_data[0] = 0x08; /* I2C Bus */
  ipmi_tx_data[1] = 0x60; /* FPGA addr */
  ipmi_tx_data[2] = 0x01; /* No of bytes to read */
  ipmi_tx_data[3] = 0x1e; /* led_ctrl1 register*/

  status = ipmi_send_message(&data->ipmi, WISTRON_IPMI_RW_CMD, &ipmi_tx_data[0],
                             4, &ipmi_resp[0], sizeof(ipmi_resp));
  if (unlikely(status != 0)) {
    goto exit;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto exit;
  }
  sys1_led = ipmi_resp[0];

  /* Get sys1_led data from ipmi */
  ipmi_resp[0] = '\0';
  ipmi_tx_data[0] = 0x08; /* I2C Bus */
  ipmi_tx_data[1] = 0x60; /* FPGA addr */
  ipmi_tx_data[2] = 0x01; /* No of bytes to read */
  ipmi_tx_data[3] = 0x1f; /* led_ctrl1 register*/

  status = ipmi_send_message(&data->ipmi, WISTRON_IPMI_RW_CMD, &ipmi_tx_data[0],
                             4, &ipmi_resp[0], sizeof(ipmi_resp));
  if (unlikely(status != 0)) {
    goto exit;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto exit;
  }
  sys2_led = ipmi_resp[0];

  data->last_updated = jiffies;
  data->valid = 1;

exit:
  data->led_data.led_ctrl1 = sys1_led;
  data->led_data.led_ctrl2 = sys2_led;
  return data;
}

static ssize_t show_led(struct device *dev, struct device_attribute *da,
                        char *buf) {
  struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
  int value = 0, tmp_val;
  int error = 0;

  mutex_lock(&data->update_lock);

  data = wistron_wtp_01_02_00_led_update_device();
  if (!data->valid) {
    error = -EIO;
    goto exit;
  }

  switch (attr->index) {
    case LED_SYS:
      if ((tmp_val = data->led_data.led_ctrl1 & 0x0C)) {
        if (tmp_val == 0x0C)
          value = LED_MODE_RED;
        else if (tmp_val == 0x04)
          value = LED_MODE_RED_BLINKING;
        else
          value = LED_MODE_OFF;
      } else if ((tmp_val = data->led_data.led_ctrl1 & 0x03)) {
        if (tmp_val == 0x03)
          value = LED_MODE_GREEN;
        else if (tmp_val == 0x01)
          value = LED_MODE_GREEN_BLINKING;
        else
          value = LED_MODE_OFF;
      } else
        value = LED_MODE_OFF;
      break;
    case LED_BMC:
      if ((tmp_val = data->led_data.led_ctrl1 & 0xC0)) {
        if (tmp_val == 0xC0)
          value = LED_MODE_RED;
        else if (tmp_val == 0x40)
          value = LED_MODE_RED_BLINKING;
        else
          value = LED_MODE_OFF;
      } else if ((tmp_val = data->led_data.led_ctrl1 & 0x30)) {
        if (tmp_val == 0x30)
          value = LED_MODE_GREEN;
        else if (tmp_val == 0x10)
          value = LED_MODE_GREEN_BLINKING;
        else
          value = LED_MODE_OFF;
      } else
        value = LED_MODE_OFF;

      break;
    case LED_FAN:
      if ((tmp_val = data->led_data.led_ctrl2 & 0x0C)) {
        if (tmp_val == 0x0C)
          value = LED_MODE_RED;
        else if (tmp_val == 0x04)
          value = LED_MODE_RED_BLINKING;
        else
          value = LED_MODE_OFF;
      } else if ((tmp_val = data->led_data.led_ctrl2 & 0x03)) {
        if (tmp_val == 0x03)
          value = LED_MODE_GREEN;
        else if (tmp_val == 0x01)
          value = LED_MODE_GREEN_BLINKING;
        else
          value = LED_MODE_OFF;
      } else
        value = LED_MODE_OFF;
      break;
    case LED_PSU:
      if ((tmp_val = data->led_data.led_ctrl2 & 0xC0)) {
        if (tmp_val == 0xC0)
          value = LED_MODE_RED;
        else if (tmp_val == 0x40)
          value = LED_MODE_RED_BLINKING;
        else
          value = LED_MODE_OFF;
      } else if ((tmp_val = data->led_data.led_ctrl2 & 0x30)) {
        if (tmp_val == 0x30)
          value = LED_MODE_GREEN;
        else if (tmp_val == 0x10)
          value = LED_MODE_GREEN_BLINKING;
        else
          value = LED_MODE_OFF;
      } else
        value = LED_MODE_OFF;

      break;

    default:
      error = -EINVAL;
      goto exit;
  }

  mutex_unlock(&data->update_lock);
  return sprintf(buf, "%d\n", value);

exit:
  mutex_unlock(&data->update_lock);
  return error;
}

static ssize_t set_led(struct device *dev, struct device_attribute *da,
                       const char *buf, size_t count) {
  long mode;
  int status, value;
  struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
  unsigned char ipmi_tx_data[5] = {0};

  status = kstrtol(buf, 10, &mode);
  if (status) {
    return status;
  }

  mutex_lock(&data->update_lock);

  data = wistron_wtp_01_02_00_led_update_device();
  if (!data->valid) {
    status = -EIO;
    goto exit;
  }

  switch (attr->index) {
    case LED_SYS:
      if (mode == LED_MODE_RED)
        value = (data->led_data.led_ctrl1 & 0xF0) | 0x0C;
      else if (mode == LED_MODE_RED_BLINKING)
        value = (data->led_data.led_ctrl1 & 0xF0) | 0x04;
      if (mode == LED_MODE_GREEN)
        value = (data->led_data.led_ctrl1 & 0xF0) | 0x03;
      else if (mode == LED_MODE_RED_BLINKING)
        value = (data->led_data.led_ctrl1 & 0xF0) | 0x01;
      else
        value = (data->led_data.led_ctrl1 & 0xF0) | 0x00;
      break;
    case LED_BMC:
      if (mode == LED_MODE_RED)
        value = (data->led_data.led_ctrl1 & 0x0F) | 0xC0;
      else if (mode == LED_MODE_RED_BLINKING)
        value = (data->led_data.led_ctrl1 & 0x0F) | 0x40;
      if (mode == LED_MODE_GREEN)
        value = (data->led_data.led_ctrl1 & 0x0F) | 0x30;
      else if (mode == LED_MODE_RED_BLINKING)
        value = (data->led_data.led_ctrl1 & 0x0F) | 0x10;
      else
        value = (data->led_data.led_ctrl1 & 0x0F) | 0x00;
      break;
    case LED_FAN:
      if (mode == LED_MODE_RED)
        value = (data->led_data.led_ctrl2 & 0xF0) | 0x0C;
      else if (mode == LED_MODE_RED_BLINKING)
        value = (data->led_data.led_ctrl2 & 0xF0) | 0x04;
      if (mode == LED_MODE_GREEN)
        value = (data->led_data.led_ctrl2 & 0xF0) | 0x03;
      else if (mode == LED_MODE_RED_BLINKING)
        value = (data->led_data.led_ctrl2 & 0xF0) | 0x01;
      else
        value = (data->led_data.led_ctrl2 & 0xF0) | 0x00;
      break;
    case LED_PSU:
      if (mode == LED_MODE_RED)
        value = (data->led_data.led_ctrl2 & 0x0F) | 0xC0;
      else if (mode == LED_MODE_RED_BLINKING)
        value = (data->led_data.led_ctrl2 & 0x0F) | 0x40;
      if (mode == LED_MODE_GREEN)
        value = (data->led_data.led_ctrl2 & 0x0F) | 0x30;
      else if (mode == LED_MODE_RED_BLINKING)
        value = (data->led_data.led_ctrl2 & 0x0F) | 0x10;
      else
        value = (data->led_data.led_ctrl2 & 0x0F) | 0x00;
      break;
    default:
      status = -EINVAL;
      goto exit;
  }

  /* Send IPMI write command */
  ipmi_tx_data[0] = 0x08; /* I2C Bus */
  ipmi_tx_data[1] = 0x60; /* FPGA addr */
  ipmi_tx_data[2] = 0x01; /* No of bytes to read */
  if ((attr->index == LED_SYS) || (attr->index == LED_BMC))
    ipmi_tx_data[3] = 0x1e;
  else
    ipmi_tx_data[3] = 0x1f;
  ipmi_tx_data[4] = value; /* register value*/

  status = ipmi_send_message(&data->ipmi, WISTRON_IPMI_RW_CMD, &ipmi_tx_data[0],
                             5, NULL, 0);
  if (unlikely(status != 0)) {
    goto exit;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto exit;
  }

  status = count;

exit:
  mutex_unlock(&data->update_lock);
  return status;
}

static int wistron_wtp_01_02_00_led_probe(struct platform_device *pdev) {
  int status = -1;

  /* Register sysfs hooks */
  status = sysfs_create_group(&pdev->dev.kobj, &wistron_wtp_01_02_00_led_group);
  if (status) {
    goto exit;
  }

  dev_info(&pdev->dev, "device created\n");

  return 0;

exit:
  return status;
}

static int wistron_wtp_01_02_00_led_remove(struct platform_device *pdev) {
  sysfs_remove_group(&pdev->dev.kobj, &wistron_wtp_01_02_00_led_group);

  return 0;
}

static int __init wistron_wtp_01_02_00_led_init(void) {
  int ret;

  data = kzalloc(sizeof(struct wistron_wtp_01_02_00_led_data), GFP_KERNEL);
  if (!data) {
    ret = -ENOMEM;
    goto alloc_err;
  }

  mutex_init(&data->update_lock);
  data->valid = 0;

  ret = platform_driver_register(&wistron_wtp_01_02_00_led_driver);
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

  return 0;

ipmi_err:
  platform_device_unregister(data->pdev);
dev_reg_err:
  platform_driver_unregister(&wistron_wtp_01_02_00_led_driver);
dri_reg_err:
  kfree(data);
alloc_err:
  return ret;
}

static void __exit wistron_wtp_01_02_00_led_exit(void) {
  ipmi_destroy_user(data->ipmi.user);
  platform_device_unregister(data->pdev);
  platform_driver_unregister(&wistron_wtp_01_02_00_led_driver);
  kfree(data);
}

MODULE_AUTHOR("HarshaF1");
MODULE_DESCRIPTION("Wistron WTP-01-02-00 LED driver");
MODULE_LICENSE("GPL");

module_init(wistron_wtp_01_02_00_led_init);
module_exit(wistron_wtp_01_02_00_led_exit);
