/*
 *
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

#define DRVNAME "wtp_01_02_00_fan"
#define WISTRON_OEM_IPMI_NETFN 0x30
#define WISTRON_IPMI_RW_CMD 0x2b
#define IPMI_NETFN 0x04
#define IPMI_FAN_READ_CMD 0x2d
#define IPMI_FAN_WRITE_CMD 0x20
#define IPMI_FAN_SDR_BASE 0x10
#define IPMI_TIMEOUT (20 * HZ)

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static ssize_t set_fan(struct device *dev, struct device_attribute *da,
                       const char *buf, size_t count);
static ssize_t show_fan(struct device *dev, struct device_attribute *attr,
                        char *buf);
static int wistron_wtp_01_02_00_fan_probe(struct platform_device *pdev);
static int wistron_wtp_01_02_00_fan_remove(struct platform_device *pdev);

enum fan_id {
  FAN_1,  /* Fan-module-1 -Front*/
  FAN_2,  /* Fan-module-1 -Rear*/
  FAN_3,  /* Fan-module-2 -Front*/
  FAN_4,  /* Fan-module-2 -Rear*/
  FAN_5,  /* Fan-module-3 -Front*/
  FAN_6,  /* Fan-module-3 -Rear*/
  FAN_7,  /* Fan-module-4 -Front*/
  FAN_8,  /* Fan-module-4 -Rear*/
  FAN_9,  /* Fan-module-5 -Front*/
  FAN_10, /* Fan-module-5 -Rear*/
  NUM_OF_FAN
};

enum fan_data_index { FAN_RPM, FAN_STATUS, FAN_PWM };

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

struct ipmi_fan_sensor_data {
  unsigned char sensor_reading;
  unsigned char status;
};

struct wistron_wtp_01_02_00_fan_data {
  struct platform_device *pdev;
  struct mutex update_lock;
  char valid[NUM_OF_FAN];                 /* != 0 if registers are valid */
  unsigned long last_updated[NUM_OF_FAN]; /* In jiffies */
  struct ipmi_fan_sensor_data fan_data[NUM_OF_FAN];
  struct ipmi_data ipmi;
};

struct wistron_wtp_01_02_00_fan_data *data = NULL;

static struct platform_driver wistron_wtp_01_02_00_fan_driver = {
    .probe = wistron_wtp_01_02_00_fan_probe,
    .remove = wistron_wtp_01_02_00_fan_remove,
    .driver =
        {
            .name = DRVNAME,
            .owner = THIS_MODULE,
        },
};

#define FAN_STATUS_ATTR_ID(index) FAN##index##_STATUS
#define FAN_PWM_ATTR_ID(index) FAN##index##_PWM
#define FAN_RPM_ATTR_ID(index) FAN##index##_RPM

#define FAN_ATTR(fan_id) \
  FAN_STATUS_ATTR_ID(fan_id), FAN_RPM_ATTR_ID(fan_id), FAN_PWM_ATTR_ID(fan_id)

enum wistron_wtp_01_02_00_fan_sysfs_attrs {
  FAN_ATTR(1),
  FAN_ATTR(2),
  FAN_ATTR(3),
  FAN_ATTR(4),
  FAN_ATTR(5),
  FAN_ATTR(6),
  FAN_ATTR(7),
  FAN_ATTR(8),
  FAN_ATTR(9),
  FAN_ATTR(10),
  NUM_OF_FAN_ATTR,
  NUM_OF_PER_FAN_ATTR = (NUM_OF_FAN_ATTR / NUM_OF_FAN)
};

/* fan attributes */
#define DECLARE_FAN_SENSOR_DEVICE_ATTR(index)                              \
  static SENSOR_DEVICE_ATTR(fan##index##_rpm, S_IRUGO, show_fan, NULL,     \
                            FAN##index##_RPM);                             \
  static SENSOR_DEVICE_ATTR(fan##index##_status, S_IRUGO, show_fan, NULL,  \
                            FAN##index##_STATUS);                          \
  static SENSOR_DEVICE_ATTR(fan##index##_pwm, S_IWUSR | S_IRUGO, show_fan, \
                            set_fan, FAN##index##_PWM)
#define DECLARE_FAN_ATTR(index)                           \
  &sensor_dev_attr_fan##index##_rpm.dev_attr.attr,        \
      &sensor_dev_attr_fan##index##_status.dev_attr.attr, \
      &sensor_dev_attr_fan##index##_pwm.dev_attr.attr

DECLARE_FAN_SENSOR_DEVICE_ATTR(1);
DECLARE_FAN_SENSOR_DEVICE_ATTR(2);
DECLARE_FAN_SENSOR_DEVICE_ATTR(3);
DECLARE_FAN_SENSOR_DEVICE_ATTR(4);
DECLARE_FAN_SENSOR_DEVICE_ATTR(5);
DECLARE_FAN_SENSOR_DEVICE_ATTR(6);
DECLARE_FAN_SENSOR_DEVICE_ATTR(7);
DECLARE_FAN_SENSOR_DEVICE_ATTR(8);
DECLARE_FAN_SENSOR_DEVICE_ATTR(9);
DECLARE_FAN_SENSOR_DEVICE_ATTR(10);

static struct attribute *wistron_wtp_01_02_00_fan_attributes[] = {
    /* fan attributes */
    DECLARE_FAN_ATTR(1),
    DECLARE_FAN_ATTR(2),
    DECLARE_FAN_ATTR(3),
    DECLARE_FAN_ATTR(4),
    DECLARE_FAN_ATTR(5),
    DECLARE_FAN_ATTR(6),
    DECLARE_FAN_ATTR(7),
    DECLARE_FAN_ATTR(8),
    DECLARE_FAN_ATTR(9),
    DECLARE_FAN_ATTR(10),
    NULL};

static const struct attribute_group wistron_wtp_01_02_00_fan_group = {
    .attrs = wistron_wtp_01_02_00_fan_attributes,
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
                             unsigned char *rx_data, unsigned short rx_len,
                             bool is_custom_cmd) {
  int err;

  if (is_custom_cmd) {
    ipmi->tx_message.netfn = WISTRON_OEM_IPMI_NETFN;
  } else
    ipmi->tx_message.netfn = IPMI_NETFN;

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

static struct wistron_wtp_01_02_00_fan_data *
wistron_wtp_01_02_00_fan_update_device(unsigned char fid) {
  int status = 0;
  unsigned char ipmi_resp[4] = {0};
  unsigned char ipmi_tx_data[4] = {0};

  if (time_before(jiffies, data->last_updated[fid] + HZ * 5) &&
      data->valid[fid]) {
    return data;
  }

  data->valid[fid] = 0;
  ipmi_tx_data[0] = IPMI_FAN_SDR_BASE + fid;
  status = ipmi_send_message(&data->ipmi, IPMI_FAN_READ_CMD, &ipmi_tx_data[0],
                             1, &ipmi_resp[0], sizeof(ipmi_resp), 0);
  if (unlikely(status != 0)) {
    goto exit;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto exit;
  }

  data->last_updated[fid] = jiffies;
  data->valid[fid] = 1;

exit:
  data->fan_data[fid].sensor_reading = ipmi_resp[0];
  data->fan_data[fid].status = ipmi_resp[2];
  return data;
}

static ssize_t show_fan(struct device *dev, struct device_attribute *da,
                        char *buf) {
  struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
  unsigned char fid = attr->index / NUM_OF_PER_FAN_ATTR;
  int value = 0;
  int error = 0;

  mutex_lock(&data->update_lock);

  data = wistron_wtp_01_02_00_fan_update_device(fid);
  if (!data->valid[fid]) {
    error = -EIO;
    goto exit;
  }

  switch (attr->index) {
    case FAN1_RPM:
    case FAN2_RPM:
    case FAN3_RPM:
    case FAN4_RPM:
    case FAN5_RPM:
    case FAN6_RPM:
    case FAN7_RPM:
    case FAN8_RPM:
    case FAN9_RPM:
    case FAN10_RPM:
      value = (int)data->fan_data[fid].sensor_reading;
      break;
    case FAN1_STATUS:
    case FAN2_STATUS:
    case FAN3_STATUS:
    case FAN4_STATUS:
    case FAN5_STATUS:
    case FAN6_STATUS:
    case FAN7_STATUS:
    case FAN8_STATUS:
    case FAN9_STATUS:
    case FAN10_STATUS:
      value = (int)data->fan_data[fid].status;
      break;
    case FAN1_PWM:
    case FAN2_PWM:
    case FAN3_PWM:
    case FAN4_PWM:
    case FAN5_PWM:
    case FAN6_PWM:
    case FAN7_PWM:
    case FAN8_PWM:
    case FAN9_PWM:
    case FAN10_PWM:
      /* Harsha - visit later for set action */
      value = (int)data->fan_data[fid].sensor_reading;
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

static ssize_t set_fan(struct device *dev, struct device_attribute *da,
                       const char *buf, size_t count) {
  long pwm_percentage;
  int status;
  struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
  unsigned char fid = attr->index / NUM_OF_PER_FAN_ATTR;
  unsigned char ipmi_tx_data[4] = {0};

  status = kstrtol(buf, 10, &pwm_percentage);
  if (status) {
    return status;
  }

  mutex_lock(&data->update_lock);

  /* Send IPMI write command */
  ipmi_tx_data[0] =
      1; /* All FANs share the same PWM register, ALWAYS set 1 for each fan */
  ipmi_tx_data[1] = pwm_percentage;
  status = ipmi_send_message(&data->ipmi, IPMI_FAN_WRITE_CMD, &ipmi_tx_data[0],
                             sizeof(ipmi_tx_data), NULL, 0, 1);
  if (unlikely(status != 0)) {
    goto exit;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto exit;
  }

  /* Update pwm to ipmi_resp buffer to prevent from the impact of lazy update */
  // data->ipmi_resp[fid] = pwm;
  status = count;

exit:
  mutex_unlock(&data->update_lock);
  return status;
}

static int wistron_wtp_01_02_00_fan_probe(struct platform_device *pdev) {
  int status = -1;

  /* Register sysfs hooks */
  status = sysfs_create_group(&pdev->dev.kobj, &wistron_wtp_01_02_00_fan_group);
  if (status) {
    goto exit;
  }

  dev_info(&pdev->dev, "device created\n");

  return 0;

exit:
  return status;
}

static int wistron_wtp_01_02_00_fan_remove(struct platform_device *pdev) {
  sysfs_remove_group(&pdev->dev.kobj, &wistron_wtp_01_02_00_fan_group);

  return 0;
}

static int __init wistron_wtp_01_02_00_fan_init(void) {
  int ret;

  data = kzalloc(sizeof(struct wistron_wtp_01_02_00_fan_data), GFP_KERNEL);
  if (!data) {
    ret = -ENOMEM;
    goto alloc_err;
  }

  mutex_init(&data->update_lock);

  ret = platform_driver_register(&wistron_wtp_01_02_00_fan_driver);
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
  platform_driver_unregister(&wistron_wtp_01_02_00_fan_driver);
dri_reg_err:
  kfree(data);
alloc_err:
  return ret;
}

static void __exit wistron_wtp_01_02_00_fan_exit(void) {
  ipmi_destroy_user(data->ipmi.user);
  platform_device_unregister(data->pdev);
  platform_driver_unregister(&wistron_wtp_01_02_00_fan_driver);
  kfree(data);
}

MODULE_AUTHOR("HarshaF1");
MODULE_DESCRIPTION("WISTRON-WTP_01_02_00 fan driver");
MODULE_LICENSE("GPL");

module_init(wistron_wtp_01_02_00_fan_init);
module_exit(wistron_wtp_01_02_00_fan_exit);
