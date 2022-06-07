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

#define DRVNAME "wtp_01_02_00_thermal"
#define WISTRON_OEM_IPMI_NETFN 0x30
#define WISTRON_IPMI_RW_CMD 0x2b
#define WISTRON_IPMI_SET_INTERNAL_SENSOR_READING 0x2f
#define IPMI_NETFN 0x04
#define IPMI_THERMAL_READ_CMD 0x2d
#define IPMI_THERMAL_TH_READ_CMD 0x27
#define IPMI_TIMEOUT (20 * HZ)

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static ssize_t show_temp(struct device *dev, struct device_attribute *attr,
                         char *buf);
static ssize_t set_temp(struct device *dev, struct device_attribute *da,
                        const char *buf, size_t count);
static int wistron_wtp_01_02_00_thermal_probe(struct platform_device *pdev);
static int wistron_wtp_01_02_00_thermal_remove(struct platform_device *pdev);

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

enum thermal_id {
  THERMAL_1,
  THERMAL_2,
  THERMAL_3,
  THERMAL_4,
  THERMAL_5,
  THERMAL_6,
  THERMAL_7,
  THERMAL_8,
  THERMAL_9,
  THERMAL_10,
  THERMAL_11,
  THERMAL_12,
  THERMAL_13,
  THERMAL_14,
  THERMAL_15,
  THERMAL_16,
  THERMAL_17,
  THERMAL_18,
  THERMAL_19,
  THERMAL_20,
  NUM_OF_THERMAL
};

int fid_thermal_sensor_no_mapping[NUM_OF_THERMAL] = {
    0x20 /* Ambient Temp   */, 0x21 /* Switch Temp    */,
    0x22 /* Sw Outlet Temp */, 0x23 /* Sw Inlet Temp  */,
    0x24 /* Sw Zone Temp   */, 0x25 /* CPU Temp       */,
    0x26 /* CPU Inlet Temp */, 0x27 /* DIMM Temp      */,
    0x28 /* VR Temp        */, 0x29 /* PIU DSP1 Temp  */,
    0x2a /* PIU DSP2 Temp  */, 0x2b /* PIU DSP3 Temp  */,
    0x2c /* PIU DSP4 Temp  */, 0x2d /* M.2 Temp       */,
    0x2e /* PSU1 Temp      */, 0x2f /* PSU2 Temp      */,
    0x32 /* ACO1 Temp      */, 0x33 /* ACO2 Temp      */,
    0x34 /* ACO3 Temp      */, 0x35 /* ACO4 Temp      */};

struct ipmi_temp_sensor_data {
  unsigned char thresh_caps; /* Capability : lnc, lcr, lnr, unc, ucr, unr */
  unsigned char sensor_reading;
  unsigned char sensor_thresholds[6];
};

struct wistron_wtp_01_02_00_thermal_data {
  struct platform_device *pdev;
  struct mutex update_lock;
  char valid[NUM_OF_THERMAL];                 /* != 0 if registers are valid */
  unsigned long last_updated[NUM_OF_THERMAL]; /* In jiffies */
  struct ipmi_temp_sensor_data thermal_data[NUM_OF_THERMAL];
  struct ipmi_data ipmi;
};

struct wistron_wtp_01_02_00_thermal_data *g_data = NULL;

static struct platform_driver wistron_wtp_01_02_00_thermal_driver = {
    .probe = wistron_wtp_01_02_00_thermal_probe,
    .remove = wistron_wtp_01_02_00_thermal_remove,
    .driver =
        {
            .name = DRVNAME,
            .owner = THIS_MODULE,
        },
};

#define TEMP_INPUT_ATTR_ID(index) TEMP##index##_INPUT
#define TEMP_CAPS_ATTR_ID(index) TEMP##index##_THRESH_CAPS
#define TEMP_THRESH_ATTR_ID(index) TEMP##index##_THRESH

#define TEMP_ATTR(_id) \
  TEMP_INPUT_ATTR_ID(_id), TEMP_CAPS_ATTR_ID(_id), TEMP_THRESH_ATTR_ID(_id)

enum wistron_wtp_01_02_00_thermal_sysfs_attrs {
  TEMP_ATTR(1),
  TEMP_ATTR(2),
  TEMP_ATTR(3),
  TEMP_ATTR(4),
  TEMP_ATTR(5),
  TEMP_ATTR(6),
  TEMP_ATTR(7),
  TEMP_ATTR(8),
  TEMP_ATTR(9),
  TEMP_ATTR(10),
  TEMP_ATTR(11),
  TEMP_ATTR(12),
  TEMP_ATTR(13),
  TEMP_ATTR(14),
  TEMP_ATTR(15),
  TEMP_ATTR(16),
  TEMP_ATTR(17),
  TEMP_ATTR(18),
  TEMP_ATTR(19),
  TEMP_ATTR(20),
  NUM_OF_THERMAL_ATTR,
  NUM_OF_PER_THERMAL_ATTR = (NUM_OF_THERMAL_ATTR / NUM_OF_THERMAL),
  BMC_SENSOR_READING
};

/* thermal attributes */
#define DECLARE_TEMP_SENSOR_DEVICE_ATTR(index)                              \
  static SENSOR_DEVICE_ATTR(temp##index##_input, S_IRUGO, show_temp, NULL,  \
                            TEMP##index##_INPUT);                           \
  static SENSOR_DEVICE_ATTR(temp##index##_thresh_caps, S_IRUGO, show_temp,  \
                            NULL, TEMP##index##_THRESH_CAPS);               \
  static SENSOR_DEVICE_ATTR(temp##index##_thresh, S_IRUGO, show_temp, NULL, \
                            TEMP##index##_THRESH)
#define DECLARE_TEMP_ATTR(index)                                \
  &sensor_dev_attr_temp##index##_input.dev_attr.attr,           \
      &sensor_dev_attr_temp##index##_thresh_caps.dev_attr.attr, \
      &sensor_dev_attr_temp##index##_thresh.dev_attr.attr

DECLARE_TEMP_SENSOR_DEVICE_ATTR(1);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(2);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(3);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(4);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(5);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(6);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(7);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(8);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(9);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(10);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(11);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(12);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(13);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(14);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(15);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(16);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(17);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(18);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(19);
DECLARE_TEMP_SENSOR_DEVICE_ATTR(20);

static SENSOR_DEVICE_ATTR(bmc_internal_sensor_reading, S_IRUGO | S_IWUSR, NULL,
                          set_temp, BMC_SENSOR_READING);

static struct attribute *wistron_wtp_01_02_00_thermal_attributes[] = {
    DECLARE_TEMP_ATTR(1),
    DECLARE_TEMP_ATTR(2),
    DECLARE_TEMP_ATTR(3),
    DECLARE_TEMP_ATTR(4),
    DECLARE_TEMP_ATTR(5),
    DECLARE_TEMP_ATTR(6),
    DECLARE_TEMP_ATTR(7),
    DECLARE_TEMP_ATTR(8),
    DECLARE_TEMP_ATTR(9),
    DECLARE_TEMP_ATTR(10),
    DECLARE_TEMP_ATTR(11),
    DECLARE_TEMP_ATTR(12),
    DECLARE_TEMP_ATTR(13),
    DECLARE_TEMP_ATTR(14),
    DECLARE_TEMP_ATTR(15),
    DECLARE_TEMP_ATTR(16),
    DECLARE_TEMP_ATTR(17),
    DECLARE_TEMP_ATTR(18),
    DECLARE_TEMP_ATTR(19),
    DECLARE_TEMP_ATTR(20),
    &sensor_dev_attr_bmc_internal_sensor_reading.dev_attr.attr,
    NULL};

static const struct attribute_group wistron_wtp_01_02_00_thermal_group = {
    .attrs = wistron_wtp_01_02_00_thermal_attributes,
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
  dev_err(&g_data->pdev->dev, "request_timeout=%x\n", err);
  return err;
ipmi_req_err:
  dev_err(&g_data->pdev->dev, "request_settime=%x\n", err);
  return err;
addr_err:
  dev_err(&g_data->pdev->dev, "validate_addr=%x\n", err);
  return err;
}

/* Dispatch IPMI messages to callers */
static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data) {
  unsigned short rx_len;
  struct ipmi_data *ipmi = user_msg_data;

  if (msg->msgid != ipmi->tx_msgid) {
    dev_err(&g_data->pdev->dev,
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

static struct wistron_wtp_01_02_00_thermal_data *
wistron_wtp_01_02_00_thermal_update_device(unsigned char fid) {
  int status = 0, temp = 0, caps = 0, thresholds[6] = {0};
  unsigned char ipmi_resp[32] = {0};
  unsigned char ipmi_tx_data[4] = {0};

  if (time_before(jiffies, g_data->last_updated[fid] + HZ * 5) &&
      g_data->valid[fid]) {
    return g_data;
  }

  g_data->valid[fid] = 0;

  ipmi_tx_data[0] = fid_thermal_sensor_no_mapping[fid];
  status =
      ipmi_send_message(&g_data->ipmi, IPMI_THERMAL_READ_CMD, &ipmi_tx_data[0],
                        1, &ipmi_resp[0], sizeof(ipmi_resp), 0);
  if (status == 0) {
    temp = ipmi_resp[0];
  }

  if (unlikely(status != 0)) {
    goto exit;
  }

  if (unlikely(g_data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto exit;
  }

  ipmi_tx_data[0] = fid_thermal_sensor_no_mapping[fid];
  status = ipmi_send_message(&g_data->ipmi, IPMI_THERMAL_TH_READ_CMD,
                             &ipmi_tx_data[0], 1, &ipmi_resp[0],
                             sizeof(ipmi_resp), 0);
  if (status == 0) {
    caps = ipmi_resp[0];
    memcpy(&thresholds[0], &ipmi_resp[1], 6);
  }

  if (unlikely(status != 0)) {
    goto exit;
  }

  if (unlikely(g_data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto exit;
  }

  g_data->last_updated[fid] = jiffies;
  g_data->valid[fid] = 1;

exit:
  g_data->thermal_data[fid].sensor_reading = temp;
  g_data->thermal_data[fid].thresh_caps = caps;
  memcpy(&g_data->thermal_data[fid].sensor_thresholds[0], &thresholds[0], 6);
  return g_data;
}

static ssize_t show_temp(struct device *dev, struct device_attribute *da,
                         char *buf) {
  int value = 0, retv = 0;
  int error = 0;
  struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
  unsigned char fid = attr->index / NUM_OF_PER_THERMAL_ATTR;

  mutex_lock(&g_data->update_lock);

  g_data = wistron_wtp_01_02_00_thermal_update_device(fid);
  if (!g_data->valid[fid]) {
    error = -EIO;
    goto exit;
  }

  switch (attr->index) {
    case TEMP1_INPUT:
    case TEMP2_INPUT:
    case TEMP3_INPUT:
    case TEMP4_INPUT:
    case TEMP5_INPUT:
    case TEMP6_INPUT:
    case TEMP7_INPUT:
    case TEMP8_INPUT:
    case TEMP9_INPUT:
    case TEMP10_INPUT:
    case TEMP11_INPUT:
    case TEMP12_INPUT:
    case TEMP13_INPUT:
    case TEMP14_INPUT:
    case TEMP15_INPUT:
    case TEMP16_INPUT:
    case TEMP17_INPUT:
    case TEMP18_INPUT:
    case TEMP19_INPUT:
    case TEMP20_INPUT:
      value = g_data->thermal_data[fid].sensor_reading;
      retv = sprintf(buf, "%d\n", value);
      break;
    case TEMP1_THRESH_CAPS:
    case TEMP2_THRESH_CAPS:
    case TEMP3_THRESH_CAPS:
    case TEMP4_THRESH_CAPS:
    case TEMP5_THRESH_CAPS:
    case TEMP6_THRESH_CAPS:
    case TEMP7_THRESH_CAPS:
    case TEMP8_THRESH_CAPS:
    case TEMP9_THRESH_CAPS:
    case TEMP10_THRESH_CAPS:
    case TEMP11_THRESH_CAPS:
    case TEMP12_THRESH_CAPS:
    case TEMP13_THRESH_CAPS:
    case TEMP14_THRESH_CAPS:
    case TEMP15_THRESH_CAPS:
    case TEMP16_THRESH_CAPS:
    case TEMP17_THRESH_CAPS:
    case TEMP18_THRESH_CAPS:
    case TEMP19_THRESH_CAPS:
    case TEMP20_THRESH_CAPS:
      value = g_data->thermal_data[fid].thresh_caps;
      retv = sprintf(buf, "%d\n", value);
      break;
    case TEMP1_THRESH:
    case TEMP2_THRESH:
    case TEMP3_THRESH:
    case TEMP4_THRESH:
    case TEMP5_THRESH:
    case TEMP6_THRESH:
    case TEMP7_THRESH:
    case TEMP8_THRESH:
    case TEMP9_THRESH:
    case TEMP10_THRESH:
    case TEMP11_THRESH:
    case TEMP12_THRESH:
    case TEMP13_THRESH:
    case TEMP14_THRESH:
    case TEMP15_THRESH:
    case TEMP16_THRESH:
    case TEMP17_THRESH:
    case TEMP18_THRESH:
    case TEMP19_THRESH:
    case TEMP20_THRESH:
      retv = sprintf(buf, "%d\n%d\n%d\n%d\n%d\n%d\n",
                     g_data->thermal_data[fid].sensor_thresholds[0],
                     g_data->thermal_data[fid].sensor_thresholds[1],
                     g_data->thermal_data[fid].sensor_thresholds[2],
                     g_data->thermal_data[fid].sensor_thresholds[3],
                     g_data->thermal_data[fid].sensor_thresholds[4],
                     g_data->thermal_data[fid].sensor_thresholds[5]);
      break;
    default:
      error = -EINVAL;
      goto exit;
  }

  mutex_unlock(&g_data->update_lock);

  return retv;

exit:
  mutex_unlock(&g_data->update_lock);
  return error;
}

static ssize_t set_temp(struct device *dev, struct device_attribute *da,
                        const char *buf, size_t count) {
  unsigned char ipmi_tx_data[29] = {};
  unsigned char ipmi_resp = NULL;
  int i, status, data;
  char bbuf[256] = {}, *head = bbuf, **p = &head, *q = NULL;
  strncpy(bbuf, buf, count);

  for (i = 0; i < 28; i++) {
    q = strsep(p, " ");
    if (q == NULL) {
      break;
    }
    status = kstrtouint(q, 0, &data);
    if (status) {
      return status;
    }
    ipmi_tx_data[i] = data;
  }

  mutex_lock(&g_data->update_lock);

  status =
      ipmi_send_message(&g_data->ipmi, WISTRON_IPMI_SET_INTERNAL_SENSOR_READING,
                        ipmi_tx_data, 28, &ipmi_resp, 1, 1);

  mutex_unlock(&g_data->update_lock);

  if (status == 0) {
    return count;
  }
  return status;
}

static int wistron_wtp_01_02_00_thermal_probe(struct platform_device *pdev) {
  int status = -1;
  /* Register sysfs hooks */
  status =
      sysfs_create_group(&pdev->dev.kobj, &wistron_wtp_01_02_00_thermal_group);
  if (status) {
    goto exit;
  }

  dev_info(&pdev->dev, "device created\n");

  return 0;

exit:
  return status;
}

static int wistron_wtp_01_02_00_thermal_remove(struct platform_device *pdev) {
  sysfs_remove_group(&pdev->dev.kobj, &wistron_wtp_01_02_00_thermal_group);

  return 0;
}

static int __init wistron_wtp_01_02_00_thermal_init(void) {
  int ret;

  g_data =
      kzalloc(sizeof(struct wistron_wtp_01_02_00_thermal_data), GFP_KERNEL);
  if (!g_data) {
    ret = -ENOMEM;
    goto alloc_err;
  }

  mutex_init(&g_data->update_lock);

  ret = platform_driver_register(&wistron_wtp_01_02_00_thermal_driver);
  if (ret < 0) {
    goto dri_reg_err;
  }

  g_data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
  if (IS_ERR(g_data->pdev)) {
    ret = PTR_ERR(g_data->pdev);
    goto dev_reg_err;
  }

  /* Set up IPMI interface */
  ret = init_ipmi_data(&g_data->ipmi, 0, &g_data->pdev->dev);
  if (ret) goto ipmi_err;

  return 0;

ipmi_err:
  platform_device_unregister(g_data->pdev);
dev_reg_err:
  platform_driver_unregister(&wistron_wtp_01_02_00_thermal_driver);
dri_reg_err:
  kfree(g_data);
alloc_err:
  return ret;
}

static void __exit wistron_wtp_01_02_00_thermal_exit(void) {
  ipmi_destroy_user(g_data->ipmi.user);
  platform_device_unregister(g_data->pdev);
  platform_driver_unregister(&wistron_wtp_01_02_00_thermal_driver);
  kfree(g_data);
}

MODULE_AUTHOR("HarshaF1");
MODULE_DESCRIPTION("Wistron-WTP-01-02-00 Thermal driver");
MODULE_LICENSE("GPL");

module_init(wistron_wtp_01_02_00_thermal_init);
module_exit(wistron_wtp_01_02_00_thermal_exit);
