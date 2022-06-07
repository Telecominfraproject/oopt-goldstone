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

/* Some of the psu-info is missing in Hw specs from Wistron,
 * hence some part of the code is commented out */
#define DRVNAME "wtp_01_02_00_psu"
#define WISTRON_OEM_IPMI_NETFN 0x30
#define WISTRON_IPMI_RW_CMD 0x2b
#define WISTRON_IPMI_PSU_TYPE_CMD 0x35
#define IPMI_NETFN 0x04
#define IPMI_PSU_READ_CMD 0x2d
#define IPMI_PSU_TH_READ_CMD 0x27
#define IPMI_SDR_PSU_STATUS_BASE 0x06
#define IPMI_SDR_PSU_POUT_BASE 0x53
#define IPMI_SDR_PSU_TEMP_BASE 0x2E
#define IPMI_TIMEOUT (20 * HZ)

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static ssize_t show_psu(struct device *dev, struct device_attribute *attr,
                        char *buf);
static int wistron_wtp_01_02_00_psu_probe(struct platform_device *pdev);
static int wistron_wtp_01_02_00_psu_remove(struct platform_device *pdev);

static const char * PSU_TYPE_NOT_PRESENT = "not-present";
static const char * PSU_TYPE_ACBEL_AC    = "AcBel AC";
static const char * PSU_TYPE_ACBEL_DC    = "AcBel DC";
static const char * PSU_TYPE_DELTA_AC    = "Delta AC";
static const char * PSU_TYPE_DELTA_DC    = "Delta DC";
static const char * PSU_TYPE_DC12V       = "DC12V";
static const char * PSU_TYPE_UNKNOWN     = "unknown";

enum psu_id { PSU_1, PSU_2, NUM_OF_PSU };

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

struct ipmi_psu_sensor_data {
  unsigned int pout_reading;
  unsigned int pin_reading;
  unsigned int vout_reading;
  unsigned int vin_reading;
  unsigned int iout_reading;
  unsigned int iin_reading;
  unsigned char psu_temp;
  unsigned char psu_temp_thresh_caps;
  unsigned char psu_temp_thresholds[6];
  int psu_fan_rpm;
  unsigned char status;
  char model[32];
  char serial[32];
  const char *type;
};

struct wistron_wtp_01_02_00_psu_data {
  struct platform_device *pdev;
  struct mutex update_lock;
  char valid[2];                 /* != 0 if registers are valid */
  unsigned long last_updated[2]; /* In jiffies */
  struct ipmi_data ipmi;
  struct ipmi_psu_sensor_data psu_data[2];
};

struct wistron_wtp_01_02_00_psu_data *data = NULL;

static struct platform_driver wistron_wtp_01_02_00_psu_driver = {
    .probe = wistron_wtp_01_02_00_psu_probe,
    .remove = wistron_wtp_01_02_00_psu_remove,
    .driver =
        {
            .name = DRVNAME,
            .owner = THIS_MODULE,
        },
};

#define PSU_STATUS_ATTR_ID(index) PSU##index##_STATUS
#define PSU_POUT_ATTR_ID(index) PSU##index##_POUT
#define PSU_PIN_ATTR_ID(index) PSU##index##_PIN
#define PSU_VOUT_ATTR_ID(index) PSU##index##_VOUT
#define PSU_VIN_ATTR_ID(index) PSU##index##_VIN
#define PSU_IOUT_ATTR_ID(index) PSU##index##_IOUT
#define PSU_IIN_ATTR_ID(index) PSU##index##_IIN
#define PSU_MODEL_ATTR_ID(index) PSU##index##_MODEL
#define PSU_SERIAL_ATTR_ID(index) PSU##index##_SERIAL
#define PSU_TEMP_ATTR_ID(index) PSU##index##_TEMP
#define PSU_TEMP_THRESH_CAPS_ATTR_ID(index) PSU##index##_TEMP_THRESH_CAPS
#define PSU_TEMP_THRESH_ATTR_ID(index) PSU##index##_TEMP_THRESH
#define PSU_FAN_RPM_ATTR_ID(index) PSU##index##_FAN_RPM
#define PSU_TYPE_ATTR_ID(index) PSU##index##_TYPE

#define PSU_ATTR(psu_id)                                                     \
  PSU_STATUS_ATTR_ID(psu_id), PSU_POUT_ATTR_ID(psu_id),                      \
      PSU_PIN_ATTR_ID(psu_id), PSU_VOUT_ATTR_ID(psu_id),                     \
      PSU_VIN_ATTR_ID(psu_id), PSU_IOUT_ATTR_ID(psu_id),                     \
      PSU_IIN_ATTR_ID(psu_id), PSU_MODEL_ATTR_ID(psu_id),                    \
      PSU_SERIAL_ATTR_ID(psu_id), PSU_TEMP_ATTR_ID(psu_id),                  \
      PSU_TEMP_THRESH_CAPS_ATTR_ID(psu_id), PSU_TEMP_THRESH_ATTR_ID(psu_id), \
      PSU_FAN_RPM_ATTR_ID(psu_id), PSU_TYPE_ATTR_ID(psu_id)

enum wistron_wtp_01_02_00_psu_sysfs_attrs {
  /* psu attributes */
  PSU_ATTR(1),
  PSU_ATTR(2),
  NUM_OF_PSU_ATTR,
  NUM_OF_PER_PSU_ATTR = (NUM_OF_PSU_ATTR / NUM_OF_PSU)
};

/* psu attributes */
#define DECLARE_PSU_SENSOR_DEVICE_ATTR(index)                                  \
  static SENSOR_DEVICE_ATTR(psu##index##_status, S_IRUGO, show_psu, NULL,      \
                            PSU##index##_STATUS);                              \
  static SENSOR_DEVICE_ATTR(psu##index##_pout, S_IRUGO, show_psu, NULL,        \
                            PSU##index##_POUT);                                \
  static SENSOR_DEVICE_ATTR(psu##index##_pin, S_IRUGO, show_psu, NULL,         \
                            PSU##index##_PIN);                                 \
  static SENSOR_DEVICE_ATTR(psu##index##_vout, S_IRUGO, show_psu, NULL,        \
                            PSU##index##_VOUT);                                \
  static SENSOR_DEVICE_ATTR(psu##index##_vin, S_IRUGO, show_psu, NULL,         \
                            PSU##index##_VIN);                                 \
  static SENSOR_DEVICE_ATTR(psu##index##_iout, S_IRUGO, show_psu, NULL,        \
                            PSU##index##_IOUT);                                \
  static SENSOR_DEVICE_ATTR(psu##index##_iin, S_IRUGO, show_psu, NULL,         \
                            PSU##index##_IIN);                                 \
  static SENSOR_DEVICE_ATTR(psu##index##_model, S_IRUGO, show_psu, NULL,       \
                            PSU##index##_MODEL);                               \
  static SENSOR_DEVICE_ATTR(psu##index##_serial, S_IRUGO, show_psu, NULL,      \
                            PSU##index##_SERIAL);                              \
  static SENSOR_DEVICE_ATTR(psu##index##_temp, S_IRUGO, show_psu, NULL,        \
                            PSU##index##_TEMP);                                \
  static SENSOR_DEVICE_ATTR(psu##index##_temp_thresh_caps, S_IRUGO, show_psu,  \
                            NULL, PSU##index##_TEMP_THRESH_CAPS);              \
  static SENSOR_DEVICE_ATTR(psu##index##_temp_thresh, S_IRUGO, show_psu, NULL, \
                            PSU##index##_TEMP_THRESH);                         \
  static SENSOR_DEVICE_ATTR(psu##index##_fan_rpm, S_IRUGO, show_psu, NULL,     \
                            PSU##index##_FAN_RPM);                             \
  static SENSOR_DEVICE_ATTR(psu##index##_type, S_IRUGO, show_psu, NULL,        \
                            PSU##index##_TYPE)

#define DECLARE_PSU_ATTR(index)                                     \
  &sensor_dev_attr_psu##index##_status.dev_attr.attr,               \
      &sensor_dev_attr_psu##index##_pout.dev_attr.attr,             \
      &sensor_dev_attr_psu##index##_pin.dev_attr.attr,              \
      &sensor_dev_attr_psu##index##_vout.dev_attr.attr,             \
      &sensor_dev_attr_psu##index##_vin.dev_attr.attr,              \
      &sensor_dev_attr_psu##index##_iout.dev_attr.attr,             \
      &sensor_dev_attr_psu##index##_iin.dev_attr.attr,              \
      &sensor_dev_attr_psu##index##_model.dev_attr.attr,            \
      &sensor_dev_attr_psu##index##_serial.dev_attr.attr,           \
      &sensor_dev_attr_psu##index##_temp.dev_attr.attr,             \
      &sensor_dev_attr_psu##index##_temp_thresh_caps.dev_attr.attr, \
      &sensor_dev_attr_psu##index##_temp_thresh.dev_attr.attr,      \
      &sensor_dev_attr_psu##index##_fan_rpm.dev_attr.attr,          \
      &sensor_dev_attr_psu##index##_type.dev_attr.attr              \

DECLARE_PSU_SENSOR_DEVICE_ATTR(1);
DECLARE_PSU_SENSOR_DEVICE_ATTR(2);

static struct attribute *wistron_wtp_01_02_00_psu_attributes[] = {
    /* psu attributes */
    DECLARE_PSU_ATTR(1), DECLARE_PSU_ATTR(2), NULL
};

static const struct attribute_group wistron_wtp_01_02_00_psu_group = {
    .attrs = wistron_wtp_01_02_00_psu_attributes,
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

static int two_complement_to_int(u16 data, u8 valid_bit, int mask) {
  u16 valid_data = data & mask;
  bool is_negative = valid_data >> (valid_bit - 1);

  return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static int psu_param_get_linear_data(unsigned char fid, int *value,
                                     int command_code) {
  unsigned char ipmi_resp[32] = {0};
  unsigned char ipmi_tx_data[4] = {0};
  int exponent = 0, mantissa = 0, status;
  int tmp_value;

  ipmi_resp[0] = '\0';
  ipmi_tx_data[0] = 0x05; /* I2C Bus */

  if (fid == 0)
    ipmi_tx_data[1] = 0xb4; /* PSU1 address */
  else
    ipmi_tx_data[1] = 0xb6; /* PSU2 address */
  ipmi_tx_data[2] = 0x02;   /* No of bytes to read */
  ipmi_tx_data[3] = command_code;

  status = ipmi_send_message(&data->ipmi, WISTRON_IPMI_RW_CMD, &ipmi_tx_data[0],
                             4, &ipmi_resp[0], sizeof(ipmi_resp), 1);
  if (unlikely(status != 0)) {
    return status;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    return status;
  }

  tmp_value = (ipmi_resp[1] << 8) | (ipmi_resp[0]);

  exponent = two_complement_to_int(tmp_value >> 11, 5, 0x1f);
  mantissa = two_complement_to_int(tmp_value & 0x7ff, 11, 0x7ff);

  if (exponent >= 0) {
    tmp_value = (mantissa << exponent) * 1000;
  } else {
    tmp_value = ((mantissa * 1000) / (1 << -exponent));
  }

  *value = tmp_value;

  return 0;
}

static struct wistron_wtp_01_02_00_psu_data *
wistron_wtp_01_02_00_psu_update_device(unsigned char fid) {
  int status = 0, dc12v = 0;
  unsigned char ipmi_resp[32] = {0}, model_len, serial_len, temp;
  unsigned char ipmi_tx_data[4] = {0}, caps = 0, thresholds[6] = {0};
  char model[32], serial[32], *type = PSU_TYPE_NOT_PRESENT;
  int pres_status = 0, psu_fan_rpm = -1;
  int pin = -1, pout = -1, vin = -1, vout = -1, iin = -1, iout = -1;

  if (time_before(jiffies, data->last_updated[fid] + HZ * 5) &&
      data->valid[fid]) {
    return data;
  }

  data->valid[fid] = 0;

  /* Get PSU status from ipmi */
  /* FPGA can also be read to get the presence status */
  ipmi_tx_data[0] = IPMI_SDR_PSU_STATUS_BASE + fid;
  status = ipmi_send_message(&data->ipmi, IPMI_PSU_READ_CMD, &ipmi_tx_data[0],
                             1, &ipmi_resp[0], sizeof(ipmi_resp), 0);

  if (unlikely(status != 0)) {
    goto err;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto err;
  }
  pres_status = ipmi_resp[2];

  /* Get PSU type from IPMI */
  ipmi_tx_data[0] = fid;
  status = ipmi_send_message(&data->ipmi, WISTRON_IPMI_PSU_TYPE_CMD, &ipmi_tx_data[0],
                             1, &ipmi_resp[0], sizeof(ipmi_resp), 1);
  if (unlikely(status != 0)) {
    goto err;
  }
  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto err;
  }

  switch (ipmi_resp[0]) {
  case 0:
    type = PSU_TYPE_NOT_PRESENT;
    goto exit;
  case 1:
    type = PSU_TYPE_ACBEL_AC;
    break;
  case 2:
    type = PSU_TYPE_ACBEL_DC;
    break;
  case 3:
    type = PSU_TYPE_DELTA_AC;
    break;
  case 4:
    type = PSU_TYPE_DELTA_DC;
    break;
  case 5:
    type = PSU_TYPE_DC12V;
    dc12v = 1;
    break;
  default:
    type = PSU_TYPE_UNKNOWN;
    break;
  }

  ipmi_resp[0] = '\0';
  /* Get PSU pout from ipmi */
  ipmi_tx_data[0] = IPMI_SDR_PSU_POUT_BASE + fid;
  status = ipmi_send_message(&data->ipmi, IPMI_PSU_READ_CMD, &ipmi_tx_data[0],
                             1, &ipmi_resp[0], sizeof(ipmi_resp), 0);
  if (unlikely(status != 0)) {
    goto err;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto err;
  }

  /* All the psu readings are written into the file in milli */
  pout = ipmi_resp[0] * 10 * 1000;

  if (dc12v) {
    goto exit;
  }

  ipmi_resp[0] = '\0';
  /* Get PSU temp from ipmi */
  ipmi_tx_data[0] = IPMI_SDR_PSU_TEMP_BASE + fid;
  status = ipmi_send_message(&data->ipmi, IPMI_PSU_READ_CMD, &ipmi_tx_data[0],
                             1, &ipmi_resp[0], sizeof(ipmi_resp), 0);
  if (unlikely(status != 0)) {
    goto err;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto err;
  }

  temp = ipmi_resp[0];

  ipmi_resp[0] = '\0';
  /* Get PSU temp caps from ipmi */
  ipmi_tx_data[0] = IPMI_SDR_PSU_TEMP_BASE + fid;
  status =
      ipmi_send_message(&data->ipmi, IPMI_PSU_TH_READ_CMD, &ipmi_tx_data[0], 1,
                        &ipmi_resp[0], sizeof(ipmi_resp), 0);
  if (unlikely(status != 0)) {
    goto err;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto err;
  }

  caps = ipmi_resp[0];
  memcpy(&thresholds[0], &ipmi_resp[1], 6);

  /* Get PSU pIN from ipmi */
  /* PSU pIN command code = 0x97*/
  status = psu_param_get_linear_data(fid, &pin, 0x97);

  if (status != 0) {
    goto err;
  }

  /* Get PSU vIN from ipmi */
  /* PSU vIN command code = 0x88*/
  status = psu_param_get_linear_data(fid, &vin, 0x88);

  if (status != 0) {
    goto err;
  }

  /* Get PSU vOUT from ipmi */
  /* PSU vOUT command code = 0x8b*/
  status = psu_param_get_linear_data(fid, &vout, 0x8b);

  if (status != 0) {
    goto err;
  }

  /* Get PSU iIN from ipmi */
  /* PSU iIN command code = 0x89*/
  status = psu_param_get_linear_data(fid, &iin, 0x89);

  if (status != 0) {
    goto err;
  }

  /* Get PSU iOUT from ipmi */
  /* PSU iOUT command code = 0x8c*/
  status = psu_param_get_linear_data(fid, &iout, 0x8c);

  if (status != 0) {
    goto err;
  }

  /* Get PSU fan-rpm from ipmi */
  ipmi_resp[0] = '\0';
  ipmi_tx_data[0] = 0x05; /* I2C Bus */
  if (fid == 0)
    ipmi_tx_data[1] = 0xb4; /* PSU1 address */
  else
    ipmi_tx_data[1] = 0xb6; /* PSU2 address */
  ipmi_tx_data[2] = 0x02;   /* No of bytes to read */
  ipmi_tx_data[3] = 0x90;   /* PSU fan command code*/

  status = ipmi_send_message(&data->ipmi, WISTRON_IPMI_RW_CMD, &ipmi_tx_data[0],
                             4, &ipmi_resp[0], sizeof(ipmi_resp), 1);
  if (unlikely(status != 0)) {
    goto err;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto err;
  }
  psu_fan_rpm = (ipmi_resp[1] << 8) | (ipmi_resp[0]);

  /* Get PSU Model from ipmi */
  ipmi_resp[0] = '\0';
  ipmi_tx_data[0] = 0x05; /* I2C Bus */
  if (fid == 0)
    ipmi_tx_data[1] = 0xb4; /* PSU1 address */
  else
    ipmi_tx_data[1] = 0xb6; /* PSU2 address */
  ipmi_tx_data[2] = 0x1f;   /* No of bytes to read */
  ipmi_tx_data[3] = 0x9a;   /* PSU Model command code*/

  status = ipmi_send_message(&data->ipmi, WISTRON_IPMI_RW_CMD, &ipmi_tx_data[0],
                             4, &ipmi_resp[0], sizeof(ipmi_resp), 1);
  if (unlikely(status != 0)) {
    goto err;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto err;
  }
  model_len = ipmi_resp[0];
  memcpy(&model[0], &ipmi_resp[1], model_len);

  /* Get PSU Serial no from ipmi */
  ipmi_resp[0] = '\0';
  ipmi_tx_data[0] = 0x05; /* I2C Bus */
  if (fid == 0)
    ipmi_tx_data[1] = 0xb4; /* PSU1 address */
  else
    ipmi_tx_data[1] = 0xb6; /* PSU2 address */
  ipmi_tx_data[2] = 0x1f;   /* No of bytes to read */
  ipmi_tx_data[3] = 0x9e;   /* PSU Serial command code*/

  status = ipmi_send_message(&data->ipmi, WISTRON_IPMI_RW_CMD, &ipmi_tx_data[0],
                             4, &ipmi_resp[0], sizeof(ipmi_resp), 1);
  if (unlikely(status != 0)) {
    goto err;
  }

  if (unlikely(data->ipmi.rx_result != 0)) {
    status = -EIO;
    goto err;
  }
  serial_len = ipmi_resp[0];
  memcpy(&serial[0], &ipmi_resp[1], serial_len);

exit:
  data->last_updated[fid] = jiffies;
  data->valid[fid] = 1;

err:
  data->psu_data[fid].pout_reading = pout;
  data->psu_data[fid].pin_reading = pin;
  data->psu_data[fid].vout_reading = vout;
  data->psu_data[fid].vin_reading = vin;
  data->psu_data[fid].iout_reading = iout;
  data->psu_data[fid].iin_reading = iin;
  data->psu_data[fid].status = pres_status;
  data->psu_data[fid].psu_temp = temp;
  data->psu_data[fid].psu_temp_thresh_caps = caps;
  data->psu_data[fid].type = type;
  memcpy(&data->psu_data[fid].psu_temp_thresholds[0], &thresholds[0], 6);
  data->psu_data[fid].psu_fan_rpm = psu_fan_rpm;
  memcpy(&data->psu_data[fid].model[0], &model, model_len);
  memcpy(&data->psu_data[fid].serial[0], &serial, serial_len);
  return data;
}

static ssize_t show_psu(struct device *dev, struct device_attribute *da,
                        char *buf) {
  struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
  unsigned char fid = attr->index / NUM_OF_PER_PSU_ATTR;
  int value = 0;
  int error = 0;
  char *str = NULL, tmpbuf[64] = {0};
  bool print_str = 0;

  mutex_lock(&data->update_lock);

  data = wistron_wtp_01_02_00_psu_update_device(fid);
  if (!data->valid) {
    error = -EIO;
    goto exit;
  }

  switch (attr->index) {
    case PSU1_STATUS:
    case PSU2_STATUS:
      value = data->psu_data[fid].status;
      break;
    case PSU1_POUT:
    case PSU2_POUT:
      value = data->psu_data[fid].pout_reading;
      break;
    case PSU1_PIN:
    case PSU2_PIN:
      value = data->psu_data[fid].pin_reading;
      break;
    case PSU1_VOUT:
    case PSU2_VOUT:
      value = data->psu_data[fid].vout_reading;
      break;
    case PSU1_VIN:
    case PSU2_VIN:
      value = data->psu_data[fid].vin_reading;
      break;
    case PSU1_IOUT:
    case PSU2_IOUT:
      value = data->psu_data[fid].iout_reading;
      break;
    case PSU1_IIN:
    case PSU2_IIN:
      value = data->psu_data[fid].iin_reading;
      break;
    case PSU1_FAN_RPM:
    case PSU2_FAN_RPM:
      value = data->psu_data[fid].psu_fan_rpm;
      break;
    case PSU1_TEMP:
    case PSU2_TEMP:
      value = data->psu_data[fid].psu_temp;
      break;
    case PSU1_TEMP_THRESH_CAPS:
    case PSU2_TEMP_THRESH_CAPS:
      value = data->psu_data[fid].psu_temp_thresh_caps;
      break;
    case PSU1_TEMP_THRESH:
    case PSU2_TEMP_THRESH:
      str = &tmpbuf[0];
      sprintf(str, "%d\n%d\n%d\n%d\n%d\n%d",
              data->psu_data[fid].psu_temp_thresholds[0],
              data->psu_data[fid].psu_temp_thresholds[1],
              data->psu_data[fid].psu_temp_thresholds[2],
              data->psu_data[fid].psu_temp_thresholds[3],
              data->psu_data[fid].psu_temp_thresholds[4],
              data->psu_data[fid].psu_temp_thresholds[5]);
      print_str = 1;
      break;
    case PSU1_MODEL:
    case PSU2_MODEL:
      str = data->psu_data[fid].model;
      print_str = 1;
      break;
    case PSU1_SERIAL:
    case PSU2_SERIAL:
      str = data->psu_data[fid].serial;
      print_str = 1;
      break;
    case PSU1_TYPE:
    case PSU2_TYPE:
      str = data->psu_data[fid].type;
      print_str = 1;
      break;
    default:
      error = -EINVAL;
      goto exit;
  }

  mutex_unlock(&data->update_lock);
  if (print_str) {
    return sprintf(buf, "%s\n", str);
  } else {
    if ( value < 0 ) {
      error = -EINVAL;
      goto exit;
    }
    return sprintf(buf, "%d\n", value);
  }

exit:
  mutex_unlock(&data->update_lock);
  return error;
}

static int wistron_wtp_01_02_00_psu_probe(struct platform_device *pdev) {
  int status = -1;

  /* Register sysfs hooks */
  status = sysfs_create_group(&pdev->dev.kobj, &wistron_wtp_01_02_00_psu_group);
  if (status) {
    goto exit;
  }

  dev_info(&pdev->dev, "device created\n");

  return 0;

exit:
  return status;
}

static int wistron_wtp_01_02_00_psu_remove(struct platform_device *pdev) {
  sysfs_remove_group(&pdev->dev.kobj, &wistron_wtp_01_02_00_psu_group);
  return 0;
}

static int __init wistron_wtp_01_02_00_psu_init(void) {
  int ret;

  data = kzalloc(sizeof(struct wistron_wtp_01_02_00_psu_data), GFP_KERNEL);
  if (!data) {
    ret = -ENOMEM;
    goto alloc_err;
  }

  mutex_init(&data->update_lock);

  ret = platform_driver_register(&wistron_wtp_01_02_00_psu_driver);
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
  platform_driver_unregister(&wistron_wtp_01_02_00_psu_driver);
dri_reg_err:
  kfree(data);
alloc_err:
  return ret;
}

static void __exit wistron_wtp_01_02_00_psu_exit(void) {
  ipmi_destroy_user(data->ipmi.user);
  platform_device_unregister(data->pdev);
  platform_driver_unregister(&wistron_wtp_01_02_00_psu_driver);
  kfree(data);
}

MODULE_AUTHOR("HarshaF1");
MODULE_DESCRIPTION("WISTRON WTP-01-02-00 PSU driver");
MODULE_LICENSE("GPL");

module_init(wistron_wtp_01_02_00_psu_init);
module_exit(wistron_wtp_01_02_00_psu_exit);
