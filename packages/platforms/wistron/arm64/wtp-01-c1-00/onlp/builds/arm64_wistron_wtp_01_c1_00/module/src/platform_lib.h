/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *        http://www.eclipse.org/legal/epl-v10.html
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the
 * License.
 *
 * </bsn.cl>
 ************************************************************
 *
 *
 *
 ***********************************************************/
#ifndef __PLATFORM_LIB_H__
#define __PLATFORM_LIB_H__

#include <onlplib/shlocks.h>
#include <errno.h>
#include "arm64_wistron_wtp_01_c1_00_log.h"

#define CHASSIS_FAN_COUNT     6
#define CHASSIS_THERMAL_COUNT 9
//#define CHASSIS_LED_COUNT     4
// not count SFP1 /SFP2
#define CHASSIS_LED_COUNT     2
#define CHASSIS_PSU_COUNT     2
#define CHASSIS_MODULE_COUNT  4

#define CFP2_LED_COUNT		4
#define QSFP28_LED_COUNT	16

#define NUM_OF_PSU_ON_MAIN_BROAD	2
#define NUM_OF_FAN_ON_PSU_BROAD		3



#define PSU1_ID 1
#define PSU2_ID 2

#define PSU1_AC_PMBUS_PREFIX "/sys/bus/i2c/devices/18-0058/"
#define PSU2_AC_PMBUS_PREFIX "/sys/bus/i2c/devices/17-0059/"

#define PSU1_AC_PMBUS_NODE(node) PSU1_AC_PMBUS_PREFIX#node
#define PSU2_AC_PMBUS_NODE(node) PSU2_AC_PMBUS_PREFIX#node

#define PSU1_AC_EEPROM_PREFIX "/sys/bus/i2c/devices/18-0050/"
#define PSU2_AC_EEPROM_PREFIX "/sys/bus/i2c/devices/17-0051/"

#define PSU1_AC_EEPROM_NODE(node) PSU1_AC_EEPROM_PREFIX#node
#define PSU2_AC_EEPROM_NODE(node) PSU2_AC_EEPROM_PREFIX#node

#define FAN_SYSFS_PATH	"/sys/devices/platform/wtp_01_c1_00_fan/"
#define FAN_NODE(node)	FAN_SYSFS_PATH#node

#define PSU_SYSFS_PATH	"/sys/devices/platform/wtp_01_c1_00_psu/"

#define IDPROM_PATH "/sys/bus/i2c/devices/0-0053/eeprom"

#define PIU_SYSFS_PATH	"/sys/class/piu/"

#define I2C_BUS_0                0

#define FPGA_ADDR                0x30

#define QSFP_PRES_OFFSET1        0x14
#define QSFP_PRES_OFFSET2        0x15
#define PIU_PRES_OFFSET          0x07
#define PIU_MOD_PRES_OFFSET      0x10

#define QSFP_PORT_BEGIN        1
#define QSFP_PORT_END          12

#define PIU_PORT_BEGIN        13
#define PIU_PORT_END          20

#define GET_SLOTNO_FROM_PORT(port)  (((port - PIU_PORT_BEGIN)/2) + 1) 

#define ONLP_PSUI_SHM_KEY   (0xF001100 | ONLP_OID_TYPE_PSU)
#define SEM_LOCK    do {sem_wait(&global_psui_st->mutex);} while(0)
#define SEM_UNLOCK  do {sem_post(&global_psui_st->mutex);} while(0)


typedef enum card_type {
	CARD_TYPE_Q28,
	CARD_TYPE_DCO,
	CARD_TYPE_ACO,
	CARD_TYPE_NOT_PRESENT,
	CARD_TYPE_UNKNOWN
} card_type_t;

typedef enum psu_type {
    PSU_TYPE_UNKNOWN,
    PSU_TYPE_AC,
    PSU_TYPE_DC48V
} psu_type_t;

typedef enum port_type {
	PORT_TYPE_Q28,
	PORT_TYPE_PIU_Q28,
	PORT_TYPE_PIU_ACO_200G,
	PORT_TYPE_PIU_DCO_200G,
	PORT_TYPE_UNKNOWN
} port_type_t;

enum onlp_thermal_id
{
    THERMAL_RESERVED = 0,
    THERMAL_1_ON_SA56004,
    THERMAL_2_ON_SA56004,
    THERMAL_1_ON_TMP75,
    THERMAL_2_ON_TMP75,
    THERMAL_3_ON_TMP75,
    THERMAL_1_ON_LM95245,
    THERMAL_2_ON_LM95245,
    THERMAL_3_ON_LM95245,
    THERMAL_4_ON_LM95245,
};

enum fan_id {
        FAN1_ON_PSU1 = 1,
        FAN2_ON_PSU1,
        FAN3_ON_PSU1,
        FAN1_ON_PSU2,
        FAN2_ON_PSU2,
        FAN3_ON_PSU2
};

enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_SYS,
    LED_PSU,
//    LED_SFP1,
//    LED_SFP2,
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

enum led_light_mode {
    LED_MODE_OFF,
    LED_MODE_SFP_GREEN		= 0x00001,
    LED_MODE_SFP_YELLOW		= 0x00002,
    LED_MODE_SFP_GREEN_BLINKING = 0x10001,
    LED_MODE_SFP_YELLOW_BLINKING   = 0x10002,
    LED_MODE_PSU_GREEN          = 0x00001,
    LED_MODE_PSU_RED            = 0x00002,
    LED_MODE_SYS_GREEN          = 0x00001,
    LED_MODE_SYS_RED            = 0x00002,
    LED_MODE_PSU_GREEN_BLINKING = 0x10001,
    LED_MODE_PSU_RED_BLINKING   = 0x10002,
    LED_MODE_SYS_GREEN_BLINKING = 0x10001,
    LED_MODE_SYS_RED_BLINKING   = 0x10002,
    LED_MODE_PSU_UNKNOWN        = 9999
};

//int get_port_number(void);
port_type_t get_port_type(int port);

struct dsp_ctrl_s
{
	char 		*type;
	unsigned int value1;
	unsigned int value2;
	unsigned int value3;
	unsigned int value4;
};

int dsp_initialize(int dsp_bus);
int dsp_exec_read(int bus, struct dsp_ctrl_s* dsp_ctrl, int *ret_val);
int cfp_eeprom_read(int port, uint8_t data[256]);
int
fpga_get_offset_for_xvr_presence (int port, int *register_offset,
	     	                  int *presence_offset);

typedef struct _fpga_context_t {
    int fd;
    void* map_base;
    int map_size;
    off_t target;
    off_t target_base;
    int type_width;
    int items_count;
} fpga_context_t;

int fpga_open(fpga_context_t* fpga, off_t offset, int type_width, int items_count);
int fpga_close(fpga_context_t* fpga);

int fpga_read(int offset, int *value);
int fpga_write(int offset, int value);

int fpga_set_bit_low(uint16_t offset, uint16_t bit);
int fpga_set_bit_high(uint16_t offset, uint16_t bit);

int get_module_status (int slotno);
int get_piu_presence (int slotno);
int onlp_sfpi_is_present(int port);

#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
	#define DEBUG_PRINT(fmt, args...)                                        \
		printf("%s:%s[%d]: " fmt "\r\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#else
	#define DEBUG_PRINT(fmt, args...)
#endif

#endif  /* __PLATFORM_LIB_H__ */

