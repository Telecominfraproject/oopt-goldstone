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

#include "x86_64_wistron_wtp_01_02_00_log.h"

#define CHASSIS_FAN_COUNT 10
#define CHASSIS_THERMAL_COUNT 20
#define CHASSIS_LED_COUNT 4
#define CHASSIS_PSU_COUNT 2
#define CHASSIS_MODULE_COUNT 4

#define PSU1_ID 1
#define PSU2_ID 2

#define FAN_SYSFS_PATH "/sys/devices/platform/wtp_01_02_00_fan/"
#define FAN_NODE(node) FAN_SYSFS_PATH #node

#define PSU_SYSFS_PATH "/sys/devices/platform/wtp_01_02_00_psu/"

#define IDPROM_PATH "/sys/bus/i2c/devices/0-0054/eeprom"
#define SYSFPGA_PATH "/sys/bus/i2c/devices/0-0030/fpga"

#define PIU_SYSFS_PATH "/sys/class/piu/"

#define THERMAL_SYSFS_PATH "/sys/devices/platform/wtp_01_02_00_thermal/"
#define THRESHOLD_LO_NC (1 << 0)
#define THRESHOLD_LO_CR (1 << 1)
#define THRESHOLD_LO_NR (1 << 2)
#define THRESHOLD_HI_NC (1 << 3)
#define THRESHOLD_HI_CR (1 << 4)
#define THRESHOLD_HI_NR (1 << 5)

#define THRESHOLD_MAX 6

#define QSFP_PRES_OFFSET1 0x14
#define QSFP_PRES_OFFSET2 0x15
#define PIU_PRES_OFFSET 0x07
#define PIU_MOD_PRES_OFFSET 0x10

#define QSFP_PORT_BEGIN 1
#define QSFP_PORT_END 12

#define PIU_PORT_BEGIN 13
#define PIU_PORT_END 20

#define GET_SLOTNO_FROM_PORT(port) (((port - PIU_PORT_BEGIN) / 2) + 1)

typedef enum card_type {
  CARD_TYPE_Q28,
  CARD_TYPE_DCO,
  CARD_TYPE_ACO,
  CARD_TYPE_NOT_PRESENT,
  CARD_TYPE_UNKNOWN
} card_type_t;

enum onlp_thermal_id {
  THERMAL_RESERVED = 0,
  THERMAL_1_ON_MAIN_BROAD,
  THERMAL_2_ON_MAIN_BROAD,
  THERMAL_3_ON_MAIN_BROAD,
  THERMAL_4_ON_MAIN_BROAD,
  THERMAL_5_ON_MAIN_BROAD,
  THERMAL_6_ON_MAIN_BROAD,
  THERMAL_7_ON_MAIN_BROAD,
  THERMAL_8_ON_MAIN_BROAD,
  THERMAL_9_ON_MAIN_BROAD,
  THERMAL_10_ON_MAIN_BROAD,
  THERMAL_11_ON_MAIN_BROAD,
  THERMAL_12_ON_MAIN_BROAD,
  THERMAL_13_ON_MAIN_BROAD,
  THERMAL_14_ON_MAIN_BROAD,
  THERMAL_15_ON_MAIN_BROAD,
  THERMAL_16_ON_MAIN_BROAD,
  THERMAL_17_ON_MAIN_BROAD,
  THERMAL_18_ON_MAIN_BROAD,
  THERMAL_19_ON_MAIN_BROAD,
  THERMAL_20_ON_MAIN_BROAD,
  THERMAL_1_ON_PSU1,
  THERMAL_1_ON_PSU2,
};

typedef enum port_type {
  PORT_TYPE_Q28,
  PORT_TYPE_PIU_Q28,
  PORT_TYPE_PIU_ACO_200G,
  PORT_TYPE_PIU_DCO_200G,
  PORT_TYPE_UNKNOWN
} port_type_t;

port_type_t get_port_type(int port);

int get_module_status(int slotno);

int get_sff_presence(int port);

int get_piu_presence(int slotno);

int get_psu_presence(int idx);
#define DEBUG_MODE 0

#if (DEBUG_MODE == 1)
#define DEBUG_PRINT(fmt, args...) \
  printf("%s:%s[%d]: " fmt "\r\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

#endif /* __PLATFORM_LIB_H__ */
