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
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/fcntl.h>
#include <linux/unistd.h>

#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/errno.h>

#include <linux/net.h>
#include <linux/socket.h>
#include <net/sock.h>
#include <asm-generic/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>

/* Some of the psu-info is missing in Hw specs from Wistron,
 * hence some part of the code is commented out */
#define DRVNAME "wtp_01_c1_00_psu"

#define CAN_BUFFER_SIZE	8

static ssize_t show_psu(struct device *dev, struct device_attribute *attr,
                        char *buf);
static ssize_t set_psu(struct device *dev, struct device_attribute *da,
                       const char *buf, size_t count);
 static int wistron_wtp_01_c1_00_psu_probe(struct platform_device *pdev);
static int wistron_wtp_01_c1_00_psu_remove(struct platform_device *pdev);

static const char * PSU_TYPE_NOT_PRESENT = "not-present";
static const char * PSU_TYPE_AC   	 = "AC PSU";
static const char * PSU_TYPE_DC48V       = "DC12V PSU";
static const char * PSU_TYPE_UNKNOWN     = "unknown";

enum psu_id { PSU_1, PSU_2, NUM_OF_PSU };


struct psu_sensor_data {
//  unsigned int pout_reading;
//  unsigned int pin_reading;
  unsigned int vout_reading;
  unsigned int vin_reading;
  unsigned int iout_reading;
  unsigned int iin_reading;
  unsigned char psu_temp;
//  unsigned char psu_temp_thresh_caps;
//  unsigned char psu_temp_thresholds[6];
  int psu_fan1_rpm;
  int psu_fan2_rpm;
  int psu_fan3_rpm;
  int psu_fan_pwm;
  unsigned char status;
  char serial[32];
  const char *type;
};

struct wistron_wtp_01_c1_00_psu_data {
  struct platform_device *pdev;
  struct mutex update_lock;
  char valid[2];                 /* != 0 if registers are valid */
  unsigned long last_updated[2]; /* In jiffies */
  struct psu_sensor_data psu_data[2];
};

struct wistron_wtp_01_c1_00_psu_data *data = NULL;

static struct platform_driver wistron_wtp_01_c1_00_psu_driver = {
    .probe = wistron_wtp_01_c1_00_psu_probe,
    .remove = wistron_wtp_01_c1_00_psu_remove,
    .driver =
        {
            .name = DRVNAME,
            .owner = THIS_MODULE,
        },
};

#define PSU_STATUS_ATTR_ID(index) PSU##index##_STATUS
#define PSU_VIN_ATTR_ID(index) PSU##index##_VIN
#define PSU_VOUT_ATTR_ID(index) PSU##index##_VOUT
//#define PSU_PIN_ATTR_ID(index) PSU##index##_PIN
//#define PSU_POUT_ATTR_ID(index) PSU##index##_POUT
#define PSU_IOUT_ATTR_ID(index) PSU##index##_IOUT
#define PSU_IIN_ATTR_ID(index) PSU##index##_IIN
#define PSU_MODEL_ATTR_ID(index) PSU##index##_MODEL
#define PSU_TEMP_ATTR_ID(index) PSU##index##_TEMP
//#define PSU_TEMP_THRESH_CAPS_ATTR_ID(index) PSU##index##_TEMP_THRESH_CAPS
//#define PSU_TEMP_THRESH_ATTR_ID(index) PSU##index##_TEMP_THRESH
#define PSU_FAN1_RPM_ATTR_ID(index) PSU##index##_FAN1_RPM
#define PSU_FAN2_RPM_ATTR_ID(index) PSU##index##_FAN2_RPM
#define PSU_FAN3_RPM_ATTR_ID(index) PSU##index##_FAN3_RPM
#define PSU_FAN_PWM_ATTR_ID(index) PSU##index##_FAN_PWM
#define PSU_TYPE_ATTR_ID(index) PSU##index##_TYPE

#define PSU_ATTR(psu_id)                                                     \
  PSU_STATUS_ATTR_ID(psu_id), PSU_VOUT_ATTR_ID(psu_id),                      \
      PSU_VIN_ATTR_ID(psu_id), PSU_IOUT_ATTR_ID(psu_id),                     \
      PSU_IIN_ATTR_ID(psu_id), PSU_TEMP_ATTR_ID(psu_id),                     \
      PSU_FAN1_RPM_ATTR_ID(psu_id), PSU_FAN2_RPM_ATTR_ID(psu_id),            \
      PSU_FAN3_RPM_ATTR_ID(psu_id), PSU_FAN_PWM_ATTR_ID(psu_id),             \
      PSU_TYPE_ATTR_ID(psu_id)
/*      PSU_PIN_ATTR_ID(psu_id), PSU_POUT_ATTR_ID(psu_id),                     \ */
/*      PSU_TEMP_THRESH_CAPS_ATTR_ID(psu_id), PSU_TEMP_THRESH_ATTR_ID(psu_id), \ */
  
enum wistron_wtp_01_c1_00_psu_sysfs_attrs {
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
  static SENSOR_DEVICE_ATTR(psu##index##_vin, S_IRUGO, show_psu, NULL,        \
                            PSU##index##_VIN);                                \
  static SENSOR_DEVICE_ATTR(psu##index##_vout, S_IRUGO, show_psu, NULL,        \
                            PSU##index##_VOUT);                                \
  static SENSOR_DEVICE_ATTR(psu##index##_iin, S_IRUGO, show_psu, NULL,         \
                            PSU##index##_IIN);                                 \
  static SENSOR_DEVICE_ATTR(psu##index##_iout, S_IRUGO, show_psu, NULL,        \
                            PSU##index##_IOUT);                                \
  static SENSOR_DEVICE_ATTR(psu##index##_temp, S_IRUGO, show_psu, NULL,        \
                            PSU##index##_TEMP);                                \
  static SENSOR_DEVICE_ATTR(psu##index##_fan1_rpm, S_IRUGO, show_psu, set_psu,  \
		            PSU##index##_FAN1_RPM);                             \
  static SENSOR_DEVICE_ATTR(psu##index##_fan2_rpm, S_IRUGO, show_psu, set_psu,  \
                            PSU##index##_FAN2_RPM);                             \
  static SENSOR_DEVICE_ATTR(psu##index##_fan3_rpm, S_IRUGO, show_psu, set_psu,  \
                            PSU##index##_FAN3_RPM);                             \
  static SENSOR_DEVICE_ATTR(psu##index##_fan_pwm, S_IWUSR | S_IRUGO, show_psu, set_psu,   \
                            PSU##index##_FAN_PWM);                              \
  static SENSOR_DEVICE_ATTR(psu##index##_type, S_IRUGO, show_psu, NULL,         \
                            PSU##index##_TYPE)
/*  static SENSOR_DEVICE_ATTR(psu##index##_pin, S_IRUGO, show_psu, NULL,         \
                            PSU##index##_PIN);                                 \
  static SENSOR_DEVICE_ATTR(psu##index##_pout, S_IRUGO, show_psu, NULL,        \
                            PSU##index##_POUT);                                \
  static SENSOR_DEVICE_ATTR(psu##index##_temp_thresh_caps, S_IRUGO, show_psu,  \
                            NULL, PSU##index##_TEMP_THRESH_CAPS);              \
  static SENSOR_DEVICE_ATTR(psu##index##_temp_thresh, S_IRUGO, show_psu, NULL, \
                            PSU##index##_TEMP_THRESH);                         \ */

#define DECLARE_PSU_ATTR(index)                                     \
  &sensor_dev_attr_psu##index##_status.dev_attr.attr,               \
      &sensor_dev_attr_psu##index##_vin.dev_attr.attr,              \
       &sensor_dev_attr_psu##index##_vout.dev_attr.attr,            \
      &sensor_dev_attr_psu##index##_iin.dev_attr.attr,              \
      &sensor_dev_attr_psu##index##_iout.dev_attr.attr,             \
      &sensor_dev_attr_psu##index##_temp.dev_attr.attr,             \
      &sensor_dev_attr_psu##index##_fan1_rpm.dev_attr.attr,         \
      &sensor_dev_attr_psu##index##_fan2_rpm.dev_attr.attr,         \
      &sensor_dev_attr_psu##index##_fan3_rpm.dev_attr.attr,         \
      &sensor_dev_attr_psu##index##_fan_pwm.dev_attr.attr,          \
      &sensor_dev_attr_psu##index##_type.dev_attr.attr              \
/*      &sensor_dev_attr_psu##index##_pin.dev_attr.attr,              \
      &sensor_dev_attr_psu##index##_pout.dev_attr.attr,             \ 
      &sensor_dev_attr_psu##index##_temp_thresh_caps.dev_attr.attr, \
      &sensor_dev_attr_psu##index##_temp_thresh.dev_attr.attr,      \ */

DECLARE_PSU_SENSOR_DEVICE_ATTR(1);
DECLARE_PSU_SENSOR_DEVICE_ATTR(2);

static struct attribute *wistron_wtp_01_c1_00_psu_attributes[] = {
    /* psu attributes */
    DECLARE_PSU_ATTR(1), DECLARE_PSU_ATTR(2), NULL
};

static const struct attribute_group wistron_wtp_01_c1_00_psu_group = {
    .attrs = wistron_wtp_01_c1_00_psu_attributes,
};


int     CANID[] = { 0x200,      // RX_PDO1
                    0x180,  // TX_PDO1
                    0x280,  // TX_PDO2
                    0x380,  // TX_PDO3
                    0x480}; // TX_PDO4

struct socket 
*onlp_can_open(int psu_id, uint8_t cob)
{
    int rv;
    struct ifreq ifr;
    struct can_filter rfilter[1];
    struct sockaddr_can addr;
    int can_id;
    struct socket *sock;
    struct timeval timeout;

    if (sock_create_kern(&init_net, PF_CAN, SOCK_RAW, CAN_RAW, &sock) < 0)
    {
        dev_err(&data->pdev->dev, "sock_create_kern failed");
        goto error;
    }


    /* Set SLAVE or SLAVE_FORCE address */
    strcpy(ifr.ifr_name, "can0" );
    //ioctl(fd, SIOCGIFINDEX, &ifr);

    ifr.ifr_ifindex = 2;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if ((rv=kernel_bind(sock, (struct sockaddr *)&addr, sizeof(addr))) < 0) {
        dev_err(&data->pdev->dev, "CanBus %d: bind failed rv[%x]", psu_id, rv);
        goto error;
    }


    // set timeout to 0.3 second (300,000)
    timeout.tv_sec = 0;
    timeout.tv_usec = 300000;

    if (kernel_setsockopt (sock, SOL_SOCKET, /*SO_REUSEADDR*/SO_RCVTIMEO_OLD, (char *)&timeout, sizeof(timeout)) < 0)
    {
        dev_err(&data->pdev->dev, "setsockopt set timeout failed\n");
        goto error;
    }

    can_id = CANID[cob] + psu_id ;
    rfilter[0].can_id   = can_id;
    rfilter[0].can_mask = CAN_SFF_MASK; 
    //rfilter[0].can_mask = 0xFFF;

    rv = kernel_setsockopt(sock, SOL_CAN_RAW, CAN_RAW_FILTER, (char *)&rfilter, sizeof(rfilter));

    if(rv < 0) {
        dev_err(&data->pdev->dev, "CabBus %d: socket operation failed %x", psu_id, rv);
        goto error;
    }

    return sock;

 error:

    kernel_sock_shutdown(sock, SHUT_RDWR);
    sock_release(sock);

    return 0;
}

int
onlp_can_read(int fid, int size, uint8_t* rdata, uint32_t flags)
{
    struct socket *sock;
    int nbytes, error_count=1;
    int can_id;
    struct can_frame frame;
    struct kvec can_vec;
    struct msghdr can_msg;

    struct sockaddr_can addr;
    struct ifreq ifr;
    // fid is 0 base, psu_id is 1 base
    int    psu_id = fid+1;

    sock = onlp_can_open(psu_id, flags);
    if (sock == 0) {
        //dev_err(&data->pdev->dev, "CANBus-%d: open failed", psu_id);
        goto error;
    }

    memset(&frame, 0, sizeof(frame));
    can_id = CANID[flags] + psu_id ;

    frame.can_id = can_id;
    frame.can_dlc = 0;

    memset(&can_vec, 0, sizeof(can_vec));
    memset(&can_msg, 0, sizeof(can_msg));

    can_vec.iov_base = &frame; //can_buf;
    can_vec.iov_len = sizeof(struct can_frame);
    
    //can_msg.msg_iov = 
    strcpy(ifr.ifr_name, "can0");
    ifr.ifr_ifindex = 2;
    addr.can_ifindex = ifr.ifr_ifindex;
    addr.can_family = AF_CAN;


    if((nbytes = kernel_sendmsg(sock, &can_msg, &can_vec, 1, sizeof(struct can_frame))) < sizeof(struct can_frame))
    {
        dev_err(&data->pdev->dev, "CANBus-%d[can_id:0x%x]: sendmsg failed, rv=%x", psu_id, can_id, nbytes);
        goto error;
    }

    do {
        msleep(10*error_count);

	memset(&frame, 0, sizeof(frame));
	memset(&can_vec, 0, sizeof(can_vec));
	memset(&can_msg, 0, sizeof(can_msg));

	can_vec.iov_base = &frame;
	can_vec.iov_len = sizeof(frame);

        nbytes = kernel_recvmsg(sock, &can_msg, &can_vec, 1, sizeof(frame), MSG_DONTWAIT);

        if (frame.can_dlc != CAN_BUFFER_SIZE) {
            error_count++;
        }
	else if (frame.can_id != can_id)
	{
	    error_count++;
	}
        else
        {
	    // succeed to receiver message from PSU
            break;
        }
        if (error_count >= 5)
        {
            goto error;
        }
    } while (1);

    memcpy (rdata, frame.data, frame.can_dlc);

    kernel_sock_shutdown(sock, SHUT_RDWR);
    sock_release (sock);

    return frame.can_dlc;

 error:
    if (sock != 0)
    {
        kernel_sock_shutdown(sock, SHUT_RDWR);
        sock_release(sock);
    }

    return -1;
}

int
onlp_can_write(int fid, int size, uint8_t* rdata, uint32_t flags)
{
    struct socket *sock;
    int nbytes;
    int can_id;
    struct can_frame frame;
    struct kvec can_vec;
    struct msghdr can_msg;

    struct sockaddr_can addr;
    struct ifreq ifr;
    // fid is 0 base, psu_id is 1 base
    int    psu_id = fid+1;
 

    sock = onlp_can_open(psu_id, flags);
    if (sock == 0) {
        //dev_err(&data->pdev->dev, "CANBus-%d: open failed", psu_id);
        goto error;
    }

    can_id = CANID[flags] + psu_id ;

    // Can't exceed maximum can bus data length : CAN_MAX_DLC (8)
    size = (size > CAN_MAX_DLC) ? CAN_MAX_DLC : size;

    memset(&frame, 0, sizeof(frame));
    can_id = CANID[flags] + psu_id ;

    frame.can_id = can_id;
    frame.can_dlc = size;
    memcpy (frame.data, rdata, size);


    memset(&can_vec, 0, sizeof(can_vec));
    memset(&can_msg, 0, sizeof(can_msg));

    can_vec.iov_base = &frame; //can_buf;
    can_vec.iov_len = sizeof(struct can_frame);

    strcpy(ifr.ifr_name, "can0");
    ifr.ifr_ifindex = 2;
    addr.can_ifindex = ifr.ifr_ifindex;
    addr.can_family = AF_CAN;


    if((nbytes = kernel_sendmsg(sock, &can_msg, &can_vec, 1, sizeof(struct can_frame))) < sizeof(struct can_frame))
    {
        dev_err(&data->pdev->dev, "CANBus-%d[can_id:0x%x]: sendmsg failed, rv=%x", psu_id, can_id, nbytes);

        goto error;
    }


    kernel_sock_shutdown(sock, SHUT_RDWR);
    sock_release (sock);

    return frame.can_dlc;

 error:
    if (sock != 0)
    {
        kernel_sock_shutdown(sock, SHUT_RDWR);
        sock_release(sock);
    }

    return -1;
}






static int two_complement_to_int(u16 data, u8 valid_bit, int mask) {
  u16 valid_data = data & mask;
  bool is_negative = valid_data >> (valid_bit - 1);

  return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

int
pus_fan_pwm_set(int pid, int p)
{
    int ret=0;
    unsigned char     buffer[9];
    int flags=1;
    int error=0;

    /* reject p=0 (p=0, stop fan) */
    if (p == 0){
	error = -EINVAL;
        goto err;
    }

    // minimum PWM will be 30%
    if (p < 30) {
	p = 30;
    }

    /* Get the PSU present state */
    data->valid[pid] = 0;
    msleep(20);

    ret = onlp_can_read(pid, CAN_BUFFER_SIZE, buffer, flags);
    if (ret < 0) {
        dev_err(&data->pdev->dev, "Unable to read status from (""psu%d_fan_pwm)\r\n", pid+1);

	error = -EINVAL;
        goto err;
    }

    if (buffer[0] != p)
    {
        dev_dbg(&data->pdev->dev, "[ONLP] PSU%d_FAN speed changed - pwm[%d]\n", (pid+1), p);
 
        msleep(20);
        buffer[1] = buffer[0];
        buffer[0] = p;

        // write data set flags to 0
        flags = 0;
        if ((ret=onlp_can_write(pid, CAN_BUFFER_SIZE, buffer, flags)) < 0) {
            dev_err(&data->pdev->dev, "Unable to write data to fan%d_pwm [%x]", pid+1, ret);

    	    error = -EINVAL;
            goto err;
        }

        // need sleep to make write pwm take effect
        msleep(80);
    }

    data->last_updated[pid] = jiffies;
    data->valid[pid] = 1;

    data->psu_data[pid].psu_fan_pwm = p;

err:
    return error;
}

static struct wistron_wtp_01_c1_00_psu_data *
wistron_wtp_01_c1_00_psu_update_device(unsigned char fid) {
    unsigned char temp = 0;
//    unsigned char caps = 0, thresholds[6] = {0};
    char *type = (char *)PSU_TYPE_NOT_PRESENT;
    int pres_status = 0, psu_fan_pwm = -1, psu_type = -1;
    int psu_fan1_rpm = -1, psu_fan2_rpm = -1, psu_fan3_rpm = -1;
    int vin = -1, vout = -1, iin = -1, iout = -1;
    unsigned char     buffer[80];
    int ret = 0, flags = 1, tmp, exponent = 0, mantissa = 0;


    if (time_before(jiffies, data->last_updated[fid] + HZ * 5) && data->valid[fid]) {
        return data;
    }

    data->valid[fid] = 0;

    ret = onlp_can_read(fid, CAN_BUFFER_SIZE, buffer, flags);
    if (ret < 0) {
	// psu not present

        goto err;
    }

    pres_status = 1;  //ONLP_PSU_STATUS_PRESENT;
    psu_type = (buffer[5] & 0x03);
    psu_fan_pwm = buffer[0];

    if ( (psu_type == 0) || (psu_type ==1))
    {
	type = (char *)PSU_TYPE_DC48V;
        // Read DC power data from PDO4
        flags = 4;

        if (onlp_can_read (fid, CAN_BUFFER_SIZE, buffer, flags) > 0)
        {
            // READ_VIN
            tmp = ((buffer[1] << 8) | buffer[0]);

            exponent = two_complement_to_int(tmp >> 11, 5, 0x1f);
            mantissa = two_complement_to_int(tmp & 0x7ff, 11, 0x7ff);

            if (exponent >= 0) {
                tmp = (mantissa << exponent) * 1000;
            } else {
                tmp = ((mantissa * 1000) / (1 << -exponent));
            }

            vin = tmp;

            // READ_VOUT
            tmp = ((buffer[3] << 8) | buffer[2]);

            exponent = two_complement_to_int(tmp >> 11, 5, 0x1f);
            mantissa = two_complement_to_int(tmp & 0x7ff, 11, 0x7ff);

            if (exponent >= 0) {
                tmp = (mantissa << exponent) * 1000;
            } else {
                tmp = ((mantissa * 1000) / (1 << -exponent));
            }
            tmp = ((buffer[3] << 8) | buffer[2]);
            vout = tmp;

            // MFR_READ_IOUT
            tmp = ((buffer[5] << 8) | buffer[4]);

            exponent = two_complement_to_int(tmp >> 11, 5, 0x1f);
            mantissa = two_complement_to_int(tmp & 0x7ff, 11, 0x7ff);

            if (exponent >= 0) {
                tmp = (mantissa << exponent) * 1000;
            } else {
                tmp = ((mantissa * 1000) / (1 << -exponent));
            }
            iout = tmp;
        }
    } else if ( (psu_type == 2) || (psu_type ==3)) {
        type = (char *)PSU_TYPE_AC;

        // Read LM25066A(AC Power) data from PDO3
        flags = 3;

        if (onlp_can_read (fid, CAN_BUFFER_SIZE, buffer, flags) > 0)
        {
            // READ_VIN
            tmp = ((buffer[1] << 8) | buffer[0]);
            vin = (tmp*10000+180000)/2207;

            // READ_VOUT
            tmp = ((buffer[3] << 8) | buffer[2]);
            vout = (tmp*10000+180000)/2207;

            // MFR_READ_IIN
            tmp = (((buffer[7] << 8) | buffer[6])*1000000+31000000)/51405;
            iin = tmp;

            // READ_TEMPERTURE
            //temp = (((buffer[5] << 8) | buffer[4])*1000)/16000;
        }
    } else {
        type = (char *)PSU_TYPE_UNKNOWN;

    }

    // read rpm information , set flags = 2
    flags = 2;
    ret = onlp_can_read(fid, CAN_BUFFER_SIZE, buffer, flags);
    if (ret < 0) {
        dev_err(&data->pdev->dev, "Unable to read fan speed from (""psu%d)[%x]\r\n", fid+1, ret);

        goto err;
    }

    psu_fan1_rpm = ((buffer[1] << 8) | buffer[0]);
    psu_fan2_rpm = ((buffer[3] << 8) | buffer[2]);
    psu_fan3_rpm = ((buffer[5] << 8) | buffer[4]);
    
    
//exit:
    data->last_updated[fid] = jiffies;
    data->valid[fid] = 1;

err:
    data->psu_data[fid].vout_reading = vout;
    data->psu_data[fid].vin_reading = vin;
/*    data->psu_data[fid].pout_reading = pout;
    data->psu_data[fid].pin_reading = pin;*/
    data->psu_data[fid].iout_reading = iout;
    data->psu_data[fid].iin_reading = iin;
    data->psu_data[fid].status = pres_status;
    data->psu_data[fid].psu_temp = temp;
    //data->psu_data[fid].psu_temp_thresh_caps = caps;
    data->psu_data[fid].type = type;
    //memcpy(&data->psu_data[fid].psu_temp_thresholds[0], &thresholds[0], 6);
    data->psu_data[fid].psu_fan1_rpm = psu_fan1_rpm;
    data->psu_data[fid].psu_fan2_rpm = psu_fan2_rpm;
    data->psu_data[fid].psu_fan3_rpm = psu_fan3_rpm;
    data->psu_data[fid].psu_fan_pwm = psu_fan_pwm;
  
    return data;
}

static ssize_t show_psu(struct device *dev, struct device_attribute *da,
                        char *buf) {
  struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
  unsigned char fid = (attr->index / NUM_OF_PER_PSU_ATTR);
  int value = 0;
  int error = 0;
  char *str = NULL;//, tmpbuf[64] = {0};
  bool print_str = 0;

  mutex_lock(&data->update_lock);

  data = wistron_wtp_01_c1_00_psu_update_device(fid);
  if (!data->valid) {
    error = -EIO;
    goto exit;
  }

  switch (attr->index) {
    case PSU1_STATUS:
    case PSU2_STATUS:
      value = data->psu_data[fid].status;
      break;
    case PSU1_VOUT:
    case PSU2_VOUT:
      value = data->psu_data[fid].vout_reading;
      break;
    case PSU1_VIN:
    case PSU2_VIN:
      value = data->psu_data[fid].vin_reading;
      break;
/*
    case PSU1_POUT:
    case PSU2_POUT:
      value = data->psu_data[fid].pout_reading;
      break;
    case PSU1_PIN:
    case PSU2_PIN:
      value = data->psu_data[fid].pin_reading;
      break;
    case PSU1_IOUT:
    case PSU2_IOUT:
      value = data->psu_data[fid].iout_reading;
      break;
*/
    case PSU1_IIN:
    case PSU2_IIN:
      value = data->psu_data[fid].iin_reading;
      break;
    case PSU1_FAN1_RPM:
    case PSU2_FAN1_RPM:
      value = data->psu_data[fid].psu_fan1_rpm;
      break;
    case PSU1_FAN2_RPM:
    case PSU2_FAN2_RPM:
      value = data->psu_data[fid].psu_fan2_rpm;
      break;
    case PSU1_FAN3_RPM:
    case PSU2_FAN3_RPM:
      value = data->psu_data[fid].psu_fan3_rpm;
      break;
    case PSU1_FAN_PWM:
    case PSU2_FAN_PWM:
      value = data->psu_data[fid].psu_fan_pwm;
      break;
    case PSU1_TEMP:
    case PSU2_TEMP:
      value = data->psu_data[fid].psu_temp;
      break;
/*
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
*/      
    case PSU1_TYPE:
    case PSU2_TYPE:
      str = (char *)data->psu_data[fid].type;
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
      
      return error;
    }
    return sprintf(buf, "%d\n", value);
  }

exit:
  mutex_unlock(&data->update_lock);
  return error;
}

static ssize_t set_psu(struct device *dev, struct device_attribute *da,
                       const char *buf, size_t count) {
  long fan_pwm;
  int status, ret=0;
  struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
  unsigned char fid = (attr->index / NUM_OF_PER_PSU_ATTR);
 
  status = kstrtol(buf, 10, &fan_pwm);
  if (status) {
    return status;
  }

  mutex_lock(&data->update_lock);

  data = wistron_wtp_01_c1_00_psu_update_device(fid);
  if (!data->valid) {
    status = -EIO;
    goto exit;
  }

  ret = pus_fan_pwm_set(fid, fan_pwm);
  if (ret < 0) {
    dev_err(&data->pdev->dev, "Unable to set fan pwm from (""psu%d_fan_pwm)\r\n", fid+1);
 
    goto exit;
  }
 
  status = count;

exit:
  mutex_unlock(&data->update_lock);
  return status;
}

static int wistron_wtp_01_c1_00_psu_probe(struct platform_device *pdev) {
  int status = -1;

  /* Register sysfs hooks */
  status = sysfs_create_group(&pdev->dev.kobj, &wistron_wtp_01_c1_00_psu_group);
  if (status) {
    goto exit;
  }

  dev_info(&pdev->dev, "device created\n");

  return 0;

exit:
  return status;
}

static int wistron_wtp_01_c1_00_psu_remove(struct platform_device *pdev) {
  sysfs_remove_group(&pdev->dev.kobj, &wistron_wtp_01_c1_00_psu_group);
  return 0;
}

static int __init wistron_wtp_01_c1_00_psu_init(void) {
  int ret;

  data = kzalloc(sizeof(struct wistron_wtp_01_c1_00_psu_data), GFP_KERNEL);
  if (!data) {
    ret = -ENOMEM;
    goto alloc_err;
  }

  mutex_init(&data->update_lock);

  ret = platform_driver_register(&wistron_wtp_01_c1_00_psu_driver);
  if (ret < 0) {
    goto dri_reg_err;
  }

  data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
  if (IS_ERR(data->pdev)) {
    ret = PTR_ERR(data->pdev);
    goto dev_reg_err;
  }

  return 0;

dev_reg_err:
  platform_driver_unregister(&wistron_wtp_01_c1_00_psu_driver);
dri_reg_err:
  kfree(data);
alloc_err:
  return ret;
}

static void __exit wistron_wtp_01_c1_00_psu_exit(void) {
  platform_device_unregister(data->pdev);
  platform_driver_unregister(&wistron_wtp_01_c1_00_psu_driver);
  kfree(data);
}

MODULE_AUTHOR("HarshaF1");
MODULE_DESCRIPTION("WISTRON WTP-01-c1-00 PSU driver");
MODULE_LICENSE("GPL");

module_init(wistron_wtp_01_c1_00_psu_init);
module_exit(wistron_wtp_01_c1_00_psu_exit);
