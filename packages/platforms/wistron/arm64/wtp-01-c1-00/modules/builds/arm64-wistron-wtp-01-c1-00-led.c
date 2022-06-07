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
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/io.h>

#define DRVNAME "wtp_01_c1_00_led"

#define SET_FPGA_BIT(_reg, _mode, _bit)                 \
        fpga_read(_reg, &_value);                       \
        fpga_write(_reg, (_value | ((_mode&0x3) << _bit)))

#define CLEAR_FPGA_BITS(_reg, _bit)                 \
        fpga_read(_reg, &_value);                       \
        fpga_write(_reg, (_value & ~(0x3 << _bit)))

#define SET_QSFP_FPGA_BIT(_reg, _mode, _bit)                 \
        fpga_read(_reg, &_value);                       \
        fpga_write(_reg, (_value & ~((_mode&0x3) << _bit)))

#define CLEAR_QSFP_FPGA_BITS(_reg, _bit)                 \
        fpga_read(_reg, &_value);                       \
        fpga_write(_reg, (_value | (0x3 << _bit)))



#define PCI_VENDOR_ID_ACTEL 0x11aa
#define PCI_DEVICE_ID_ACTEL 0x1556
#define FPGA_PCIE_ADDRESS    0x8040000000
#define FPGA_PAGE_MASK  0xFFFFFFFFFFFFF000


#define SYS_LED_REG_ADDRESS  0x090000a8
#define PSU_LED_REG_ADDRESS  0x090000b8





static ssize_t set_led(struct device *dev, struct device_attribute *da,
                       const char *buf, size_t count);
static ssize_t show_led(struct device *dev, struct device_attribute *attr,
                        char *buf);
static int wistron_wtp_01_c1_00_led_probe(struct platform_device *pdev);
static int wistron_wtp_01_c1_00_led_remove(struct platform_device *pdev);


struct system_led_data {
    unsigned int led_reg_A8;
    unsigned int led_reg_B8;
    unsigned int led_reg_B0;
    unsigned int led_reg_C0;
};

struct wistron_wtp_01_c1_00_led_data {
  struct platform_device *pdev;
  struct mutex update_lock;
  char valid;                 /* != 0 if registers are valid */
  unsigned long last_updated; /* In jiffies */
  struct system_led_data led_data;
 };

struct wistron_wtp_01_c1_00_led_data *data = NULL;

static struct platform_driver wistron_wtp_01_c1_00_led_driver = {
    .probe = wistron_wtp_01_c1_00_led_probe,
    .remove = wistron_wtp_01_c1_00_led_remove,
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

enum wistron_wtp_01_c1_00_led_sysfs_attrs {
  LED_SYS,
  LED_PSU,
//  LED_SFP1,
//  LED_SFP2,
  LED_QSFP1,
  LED_QSFP2,
  LED_QSFP3,
  LED_QSFP4,
  LED_QSFP5,
  LED_QSFP6,
  LED_QSFP7,
  LED_QSFP8,
  LED_QSFP9,
  LED_QSFP10,
  LED_QSFP11,
  LED_QSFP12,
  LED_QSFP13,
  LED_QSFP14,
  LED_QSFP15,
  LED_QSFP16,  
  LED_CFP1,
  LED_CFP2, 
  LED_CFP3, 
  LED_CFP4
};

static SENSOR_DEVICE_ATTR(led_sys, S_IWUSR | S_IRUGO, show_led, set_led, LED_SYS);
static SENSOR_DEVICE_ATTR(led_psu, S_IWUSR | S_IRUGO, show_led, set_led, LED_PSU);
//static SENSOR_DEVICE_ATTR(led_sfp1, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP1);
//static SENSOR_DEVICE_ATTR(led_sfp2, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP2);
static SENSOR_DEVICE_ATTR(led_qsfp1, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP1);
static SENSOR_DEVICE_ATTR(led_qsfp2, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP2); 
static SENSOR_DEVICE_ATTR(led_qsfp3, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP3); 
static SENSOR_DEVICE_ATTR(led_qsfp4, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP4); 
static SENSOR_DEVICE_ATTR(led_qsfp5, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP5); 
static SENSOR_DEVICE_ATTR(led_qsfp6, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP6); 
static SENSOR_DEVICE_ATTR(led_qsfp7, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP7); 
static SENSOR_DEVICE_ATTR(led_qsfp8, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP8); 
static SENSOR_DEVICE_ATTR(led_qsfp9, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP9); 
static SENSOR_DEVICE_ATTR(led_qsfp10, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP10); 
static SENSOR_DEVICE_ATTR(led_qsfp11, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP11); 
static SENSOR_DEVICE_ATTR(led_qsfp12, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP12); 
static SENSOR_DEVICE_ATTR(led_qsfp13, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP13);
static SENSOR_DEVICE_ATTR(led_qsfp14, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP14);
static SENSOR_DEVICE_ATTR(led_qsfp15, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP15);
static SENSOR_DEVICE_ATTR(led_qsfp16, S_IWUSR | S_IRUGO, show_led, set_led, LED_QSFP16);
static SENSOR_DEVICE_ATTR(led_cfp1, S_IWUSR | S_IRUGO, show_led, set_led, LED_CFP1);
static SENSOR_DEVICE_ATTR(led_cfp2, S_IWUSR | S_IRUGO, show_led, set_led, LED_CFP2);
static SENSOR_DEVICE_ATTR(led_cfp3, S_IWUSR | S_IRUGO, show_led, set_led, LED_CFP3);
static SENSOR_DEVICE_ATTR(led_cfp4, S_IWUSR | S_IRUGO, show_led, set_led, LED_CFP4);


static struct attribute *wistron_wtp_01_c1_00_led_attributes[] = {
    &sensor_dev_attr_led_sys.dev_attr.attr,
    &sensor_dev_attr_led_psu.dev_attr.attr, 
//    &sensor_dev_attr_led_sfp1.dev_attr.attr,
//    &sensor_dev_attr_led_sfp2.dev_attr.attr,
    &sensor_dev_attr_led_qsfp1.dev_attr.attr,
    &sensor_dev_attr_led_qsfp2.dev_attr.attr,
    &sensor_dev_attr_led_qsfp3.dev_attr.attr,
    &sensor_dev_attr_led_qsfp4.dev_attr.attr,
    &sensor_dev_attr_led_qsfp5.dev_attr.attr,
    &sensor_dev_attr_led_qsfp6.dev_attr.attr,
    &sensor_dev_attr_led_qsfp7.dev_attr.attr,
    &sensor_dev_attr_led_qsfp8.dev_attr.attr,
    &sensor_dev_attr_led_qsfp9.dev_attr.attr,
    &sensor_dev_attr_led_qsfp10.dev_attr.attr,
    &sensor_dev_attr_led_qsfp11.dev_attr.attr,
    &sensor_dev_attr_led_qsfp12.dev_attr.attr,
    &sensor_dev_attr_led_qsfp13.dev_attr.attr,
    &sensor_dev_attr_led_qsfp14.dev_attr.attr,
    &sensor_dev_attr_led_qsfp15.dev_attr.attr,
    &sensor_dev_attr_led_qsfp16.dev_attr.attr,
    &sensor_dev_attr_led_cfp1.dev_attr.attr,
    &sensor_dev_attr_led_cfp2.dev_attr.attr,
    &sensor_dev_attr_led_cfp3.dev_attr.attr,
    &sensor_dev_attr_led_cfp4.dev_attr.attr,NULL};

static const struct attribute_group wistron_wtp_01_c1_00_led_group = {
    .attrs = wistron_wtp_01_c1_00_led_attributes,
};

typedef struct _fpga_context_t {
    void* map_base;
    int map_size;
    off_t target;
    off_t target_base;
    int type_width;
    int items_count;
} fpga_context_t;

int fpga_open(fpga_context_t* fpga, off_t target, int type_width, int items_count)
{
    int map_size = 4096UL;
    off_t target_base;

    if ( fpga == NULL ) {
        return -1;
    }
    
    target_base = target & FPGA_PAGE_MASK;
    if (target + items_count*type_width - target_base > map_size)
        map_size = target + items_count*type_width - target_base;

    fpga->map_base = ioremap(FPGA_PCIE_ADDRESS+target_base, map_size);
    if(fpga->map_base==NULL)
    {
        return -ENOMEM;
    }
    fpga->map_size = map_size;
    fpga->target = target;
    fpga->target_base = target_base;
    fpga->type_width = type_width;
    fpga->items_count = items_count;
    
    return 0;
}

int fpga_close(fpga_context_t* fpga) {
    if (fpga == NULL ) {
        return 0;
    }
    
    iounmap(fpga->map_base);
    fpga->map_base = NULL;
    fpga->map_size = 0;

    return 0;
}

int fpga_read(int offset, int *value)
{
    fpga_context_t ctx;
    int rv;
    void *virt_addr;

    rv = fpga_open(&ctx, offset, 4, 1);
    if ( rv < 0 ) return rv;

    virt_addr = ctx.map_base + ctx.target - ctx.target_base;
    *value = *((uint32_t *) virt_addr);
    
    return fpga_close(&ctx);
}

int fpga_write(int offset, int value)
{
    fpga_context_t ctx;
    int rv;
    void *virt_addr;

    rv = fpga_open(&ctx, offset, 4, 1);
    if ( rv < 0 ) return rv;
    virt_addr = ctx.map_base + ctx.target - ctx.target_base;
    *((uint32_t *) virt_addr) = value;

    return fpga_close(&ctx);
}



static struct wistron_wtp_01_c1_00_led_data *
wistron_wtp_01_c1_00_led_update_device(void) {
  int status = 0;
  int value1, value2;
  
  if (time_before(jiffies, data->last_updated + HZ * 5) && data->valid) {
    return data;
  }

  data->valid = 0;

  fpga_read(0x090000B0, &value1);
  fpga_read(0x090000C0, &value2);

  data->led_data.led_reg_B0 = value1;
  data->led_data.led_reg_C0 = value2;

  fpga_read(0x090000A8, &value1);
  fpga_read(0x090000B8, &value2);

  data->led_data.led_reg_A8 = value1;
  data->led_data.led_reg_B8 = value2;

  if (unlikely(status != 0)) {
    goto exit;
  }

  data->last_updated = jiffies;
  data->valid = 1;

exit:
  return data;
}

static ssize_t show_led(struct device *dev, struct device_attribute *da,
                        char *buf) {
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    int mode = 0, tmp_val, lid=attr->index;
    int value1 = 0, value2 = 0, error = 0;

    mutex_lock(&data->update_lock);

    data = wistron_wtp_01_c1_00_led_update_device();
    if (!data->valid) {
        error = -EIO;
        goto exit;
    }

    if ((lid >= LED_QSFP1) && (lid <= LED_QSFP8))
    {
	value1 = data->led_data.led_reg_B0;
	value2 = data->led_data.led_reg_C0;

        value1 = ~(value1 >> ((LED_QSFP8-lid)*2)) & 0x03;
        value2 = (value2 >> ((LED_QSFP8-lid)*2)) & 0x03;

	//printk("[DRV_GET_LED] lid[%x], B0[%x], C0[%x], value1[%x], value2[%x]\n", lid, data->led_data.led_reg_B0, data->led_data.led_reg_C0, value1, value2);
        // normal mode priority higher than blinking mode, clear blinking mode if normal
        if (value1)
        {
            if (value1 == 0x2)
                mode = LED_MODE_GREEN;
            else if (value1 == 0x1)
                mode = LED_MODE_YELLOW;
            else
                mode = LED_MODE_OFF;
        } else {
            if (value2 == 0x2)
                mode = LED_MODE_GREEN_BLINKING;
            else if (value2 == 0x1)
                mode = LED_MODE_YELLOW_BLINKING;
            else
                mode = LED_MODE_OFF;
        }
    }
    else if ((lid >= LED_QSFP9) && (lid <= LED_QSFP16))
    {
	value1 = data->led_data.led_reg_A8;
	value2 = data->led_data.led_reg_B8;

        value1 = ~(value1 >> ((LED_QSFP16-lid)*2+16)) & 0x03;
        value2 = (value2 >> ((LED_QSFP16-lid)*2+16)) & 0x03;

        // normal mode priority higher than blinking mode, clear blinking mode if normal
        if (value1)
        {
            if (value1 == 0x2)
                mode = LED_MODE_GREEN;
            else if (value1 == 0x1)
                mode = LED_MODE_YELLOW;
            else
                mode = LED_MODE_OFF;
	} else {
            if (value2 == 0x2)
                mode = LED_MODE_GREEN_BLINKING;
            else if (value2 == 0x1)
                mode = LED_MODE_YELLOW_BLINKING;
            else
                mode = LED_MODE_OFF;
        }
    }
    else if ((lid >= LED_CFP1) && (lid <= LED_CFP4))
    {
        value1 = data->led_data.led_reg_A8;
        value2 = data->led_data.led_reg_B8;

        value1 = (value1 >> ((LED_CFP4-lid)*2+8)) & 0x03;
        value2 = (value2 >> ((LED_CFP4-lid)*2+8)) & 0x03;

	// normal mode priority higher than blinking mode, clear blinking mode if normal
        if (value1)
        {
            if (value1 == 0x2)
                mode = LED_MODE_BLUE;
            else if (value1 == 0x1)
                mode = LED_MODE_GREEN;
            else
                mode = LED_MODE_OFF;
        } else {
            if (value2 == 0x2)
                mode = LED_MODE_BLUE_BLINKING;
            else if (value2 == 0x1)
                mode = LED_MODE_GREEN_BLINKING;
            else
                mode = LED_MODE_OFF;
        }
    }
    else
    {
      	switch (lid) {
            case LED_SYS:
		value1 = (data->led_data.led_reg_A8 >> 4);
		value2 = (data->led_data.led_reg_B8 >> 4);

		// blinking priority > normal for sys led 
		tmp_val = value2 & 0x3;
	  	if (tmp_val) {
                    if (tmp_val == 0x02)
                        mode = LED_MODE_RED_BLINKING;
                    else if (tmp_val == 0x01)
                        mode = LED_MODE_GREEN_BLINKING;
                    else
                        mode = LED_MODE_OFF;
                } else {
		    tmp_val = value1 & 0x3;
                    if (tmp_val == 0x02) 
                        mode = LED_MODE_RED;
                    else if (tmp_val == 0x01)
                        mode = LED_MODE_GREEN;
                    else
                        mode = LED_MODE_OFF;
                }

	  	break;

            case LED_PSU:
                value1 = (data->led_data.led_reg_A8 >> 6);
                value2 = (data->led_data.led_reg_B8 >> 6);
                
		// blinking priority > normal for psu led
		tmp_val = value2 & 0x3;

                if (tmp_val) {
                    if (tmp_val == 0x02)
                        mode = LED_MODE_RED_BLINKING;
                    else if (tmp_val == 0x01)
                        mode = LED_MODE_GREEN_BLINKING;
                    else
                        mode = LED_MODE_OFF;
                } else {
		    tmp_val = value1 & 0x3;	
                    if (tmp_val == 0x02)
                        mode = LED_MODE_RED;
                    else if (tmp_val == 0x01)
                        mode = LED_MODE_GREEN;
                    else
                        mode = LED_MODE_OFF;
                }

	  	break;


	    default:
                error = -EINVAL;
                goto exit;
        }
    }

    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%d\n", mode);

exit:
    mutex_unlock(&data->update_lock);
    return error;
}

static ssize_t set_led(struct device *dev, struct device_attribute *da,
                       const char *buf, size_t count) {
    long mode;
    int  rv, _value;
    int status, value, lid;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    status = kstrtol(buf, 10, &mode);

    //printk("[DRV_SET_LED] index[%x], mode[%lx], status[%x], buf[%s]\n", attr->index, mode, status, buf);
    if (status) {
        return status;
    }

    mutex_lock(&data->update_lock);

    data = wistron_wtp_01_c1_00_led_update_device();
    if (!data->valid) {
        status = -EIO;
        goto exit;
    }

    //printk("[DRV_SET_LED] index[%x], mode[%lx], A8[%x], B8[%x]\n", attr->index, mode, data->led_data.led_reg_A8, data->led_data.led_reg_B8);

    lid = attr->index;

    if ((lid >= LED_QSFP1) && (lid <= LED_QSFP8))
    {
        if (mode == LED_MODE_GREEN)
            value = 0x00002;
        else if (mode == LED_MODE_YELLOW)
            value = 0x00001;
        else if (mode == LED_MODE_GREEN_BLINKING)
            value = 0x10002;
        else if (mode == LED_MODE_YELLOW_BLINKING)
            value = 0x10001;
        else
            value = 0x00000;

        if (value & 0x10000)     // blinking mode
        {
             rv = CLEAR_QSFP_FPGA_BITS (0x090000B0, ((LED_QSFP8-lid)*2));
             rv = SET_FPGA_BIT (0x090000C0, value, ((LED_QSFP8-lid)*2));
        } else if ((value & 0x3) != LED_MODE_OFF) {
             rv = SET_QSFP_FPGA_BIT (0x090000B0, value, ((LED_QSFP8-lid)*2));
             rv = CLEAR_FPGA_BITS (0x090000C0, ((LED_QSFP8-lid)*2));
        } else {                // led off
             rv = CLEAR_QSFP_FPGA_BITS (0x090000B0, ((LED_QSFP8-lid)*2));
             rv = CLEAR_FPGA_BITS (0x090000C0, ((LED_QSFP8-lid)*2));
        }

    }
    else if ((lid >= LED_QSFP9) && (lid <= LED_QSFP16))
    {
        if (mode == LED_MODE_GREEN)
            value = 0x00002;
        else if (mode == LED_MODE_YELLOW)
            value = 0x00001;
        else if (mode == LED_MODE_GREEN_BLINKING)
            value = 0x10002;
        else if (mode == LED_MODE_YELLOW_BLINKING)
            value = 0x10001;
        else
            value = 0x00000;

	if (value & 0x10000)     // blinking mode
        {
             rv = CLEAR_QSFP_FPGA_BITS (0x090000A8, ((LED_QSFP16-lid)*2+16));
             rv = SET_FPGA_BIT (0x090000B8, value, ((LED_QSFP16-lid)*2+16));
        } else if ((value & 0x3) != LED_MODE_OFF) {
             rv = SET_QSFP_FPGA_BIT (0x090000A8, value, ((LED_QSFP16-lid)*2+16));
             rv = CLEAR_FPGA_BITS (0x090000B8, ((LED_QSFP16-lid)*2+16));
         } else {               // led off
             rv = CLEAR_QSFP_FPGA_BITS (0x090000A8, ((LED_QSFP16-lid)*2+16));
             rv = CLEAR_FPGA_BITS (0x090000B8, ((LED_QSFP16-lid)*2+16));
        }

    }
    else if ((lid >= LED_CFP1) && (lid <= LED_CFP4))
    {
        if (mode == LED_MODE_BLUE)
            value = 0x00002;
        else if (mode == LED_MODE_GREEN)
            value = 0x00001;
        else if (mode == LED_MODE_BLUE_BLINKING)
            value = 0x10002;
        else if (mode == LED_MODE_GREEN_BLINKING)
            value = 0x10001;
        else
            value = 0x00000;

	if (value & 0x10000)      // blinking mode
        {
             rv = CLEAR_FPGA_BITS (0x090000A8, ((LED_CFP4-lid)*2+8));
             rv = SET_FPGA_BIT (0x090000B8, value, ((LED_CFP4-lid)*2+8));
        } else if (value & 0x3) {
             rv = SET_FPGA_BIT (0x090000A8, value, ((LED_CFP4-lid)*2+8));
             rv = CLEAR_FPGA_BITS (0x090000B8, ((LED_CFP4-lid)*2+8));
        } else {                // led off
             rv = CLEAR_FPGA_BITS (0x090000A8, ((LED_CFP4-lid)*2+8));
             rv = CLEAR_FPGA_BITS (0x090000B8, ((LED_CFP4-lid)*2+8));
        }

    }
    else
    {
        switch (attr->index) {
            case LED_SYS:
                if (mode == LED_MODE_RED_BLINKING)
                    value = 0x10002;
                else if (mode == LED_MODE_GREEN_BLINKING)
                    value = 0x10001;
                else if (mode == LED_MODE_GREEN)
                    value = 0x00001;
                else
                    value = 0x00000;

    		if (value & 0x10000)  // blinking mode
                {
                     rv = CLEAR_FPGA_BITS (0x090000A8, 4);
                     rv = SET_FPGA_BIT (0x090000B8, value, 4);
                } else {
                     rv = SET_FPGA_BIT (0x090000A8, value, 4);
                     rv = CLEAR_FPGA_BITS (0x090000B8, 4);
                }
    
		break;

            case LED_PSU:
                if (mode == LED_MODE_RED_BLINKING)
                    value = 0x10002;
                else if (mode == LED_MODE_RED)
                    value = 0x00002;
                else if (mode == LED_MODE_GREEN)
                    value = 0x00001;
                else
                   value = 0x00000;

                if (value & 0x10000)  // blinking mode
                {
                     rv = CLEAR_FPGA_BITS (0x090000A8, 6);
                     rv = SET_FPGA_BIT (0x090000B8, value, 6);
                } else {
                     rv = SET_FPGA_BIT (0x090000A8, value, 6);
                     rv = CLEAR_FPGA_BITS (0x090000B8, 6);
                }
 	  	break;
        
	    default:
                status = -EINVAL;
                goto exit;
    	}
    }

    if (unlikely(status != 0)) {
        goto exit;
    }

    status = count;

exit:
    mutex_unlock(&data->update_lock);
    return status;
}

static int wistron_wtp_01_c1_00_led_probe(struct platform_device *pdev) {
  int status = -1;

  /* Register sysfs hooks */
  status = sysfs_create_group(&pdev->dev.kobj, &wistron_wtp_01_c1_00_led_group);
  if (status) {
    goto exit;
  }

  dev_info(&pdev->dev, "device created\n");

  return 0;

exit:
  return status;
}

static int wistron_wtp_01_c1_00_led_remove(struct platform_device *pdev) {
  sysfs_remove_group(&pdev->dev.kobj, &wistron_wtp_01_c1_00_led_group);

  return 0;
}

static int __init wistron_wtp_01_c1_00_led_init(void) {
  int ret;

  data = kzalloc(sizeof(struct wistron_wtp_01_c1_00_led_data), GFP_KERNEL);
  if (!data) {
    ret = -ENOMEM;
    goto alloc_err;
  }

  mutex_init(&data->update_lock);
  data->valid = 0;

  ret = platform_driver_register(&wistron_wtp_01_c1_00_led_driver);
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
  platform_driver_unregister(&wistron_wtp_01_c1_00_led_driver);
dri_reg_err:
  kfree(data);
alloc_err:
  return ret;
}

static void __exit wistron_wtp_01_c1_00_led_exit(void) {
  platform_device_unregister(data->pdev);
  platform_driver_unregister(&wistron_wtp_01_c1_00_led_driver);
  kfree(data);
}

MODULE_AUTHOR("HarshaF1");
MODULE_DESCRIPTION("Wistron WTP-01-c1-00 LED driver");
MODULE_LICENSE("GPL");

module_init(wistron_wtp_01_c1_00_led_init);
module_exit(wistron_wtp_01_c1_00_led_exit);
