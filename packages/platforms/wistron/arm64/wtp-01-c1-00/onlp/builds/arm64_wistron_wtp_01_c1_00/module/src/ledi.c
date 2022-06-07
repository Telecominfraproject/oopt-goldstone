/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2013 Accton Technology Corporation.
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
#include <onlplib/file.h>
#include <onlp/platformi/ledi.h>
#include "onlp_mdio.h"
#include "platform_lib.h"

#define LED_FORMAT "/sys/devices/platform/wtp_01_c1_00_led/%s"

#define SET_FPGA_BIT(_reg, _mode, _bit)			\
        fpga_read(_reg, &_value);			\
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


#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_LED(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

/* LED related data
 */
/*
enum onlp_led_id
{
    LED_RESERVED = 0,
    LED_SYS,
    LED_PSU,
    LED_SFP1,
    LED_SFP2
};
*/

/*
enum led_light_mode {
    LED_MODE_OFF,
    LED_MODE_SYS_GREEN          = 0x00001,
    LED_MODE_SYS_RED            = 0x00002,
    LED_MODE_PSU_GREEN          = 0x00001,
    LED_MODE_PSU_RED            = 0x00002,
    LED_MODE_SYS_GREEN_BLINKING = 0x10001,
    LED_MODE_SYS_RED_BLINKING   = 0x10002,
    LED_MODE_PSU_GREEN_BLINKING = 0x10001,
    LED_MODE_PSU_RED_BLINKING   = 0x10002,
    LED_MODE_UNKNOWN		= 9999
};
*/

enum piu_led_light_mode {
    LED_MODE_PIU_OFF,
    LED_MODE_PIU_GREEN		 = 0x00001,
    LED_MODE_PIU_BLUE		 = 0x00002,
    LED_MODE_PIU_GREEN_BLINKING  = 0x10001,
    LED_MODE_PIU_BLUE_BLINKING   = 0x10002,
    LED_MODE_PIU_UNKNOWN            = 9999
};


enum qsfp_light_mode {
    LED_MODE_QSFP_OFF		   = 0x00003,
    LED_MODE_QSFP_YELLOW	   = 0x00001,
    LED_MODE_QSFP_GREEN		   = 0x00002,
    LED_MODE_QSFP_YELLOW_BLINKING  = 0x10001,
    LED_MODE_QSFP_GREEN_BLINKING   = 0x10002,
    LED_MODE_QSFP_UNKNOWN          = 9999
};



typedef struct led_light_mode_map {
    enum onlp_led_id id;
    enum led_light_mode driver_led_mode;
    enum onlp_led_mode_e onlp_led_mode;
} led_light_mode_map_t;

led_light_mode_map_t led_map[] = {
{LED_SYS,  LED_MODE_OFF,             ONLP_LED_MODE_OFF},
{LED_SYS,  LED_MODE_SYS_RED,             ONLP_LED_MODE_RED},
{LED_SYS,  LED_MODE_SYS_RED_BLINKING,    ONLP_LED_MODE_RED_BLINKING},
{LED_SYS,  LED_MODE_SYS_GREEN,           ONLP_LED_MODE_GREEN},
{LED_SYS,  LED_MODE_SYS_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_PSU,  LED_MODE_OFF,             ONLP_LED_MODE_OFF},
{LED_PSU,  LED_MODE_PSU_RED,             ONLP_LED_MODE_RED},
{LED_PSU,  LED_MODE_PSU_RED_BLINKING,    ONLP_LED_MODE_RED_BLINKING},
{LED_PSU,  LED_MODE_PSU_GREEN,           ONLP_LED_MODE_GREEN},
{LED_PSU,  LED_MODE_PSU_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
/*
{LED_SFP1,  LED_MODE_OFF,             ONLP_LED_MODE_OFF},
{LED_SFP1,  LED_MODE_SFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_SFP1,  LED_MODE_SFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_SFP1,  LED_MODE_SFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_SFP1,  LED_MODE_SFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_SFP2,  LED_MODE_OFF,             ONLP_LED_MODE_OFF},
{LED_SFP2,  LED_MODE_SFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_SFP2,  LED_MODE_SFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_SFP2,  LED_MODE_SFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_SFP2,  LED_MODE_SFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
*/
{LED_QSFP1,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP1,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP1,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP1,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP1,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP2,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP2,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP2,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP2,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP2,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP3,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP3,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP3,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP3,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP3,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP4,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP4,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP4,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP4,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP4,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP5,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP5,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP5,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP5,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP5,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP6,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP6,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP6,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP6,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP6,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP7,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP7,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP7,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP7,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP7,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP8,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP8,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP8,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP8,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP8,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP9,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP9,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP9,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP9,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP9,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP10,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP10,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP10,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP10,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP10,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP11,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP11,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP11,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP11,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP11,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP12,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP12,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP12,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP12,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP12,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP13,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP13,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP13,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP13,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP13,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP14,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP14,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP14,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP14,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP14,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP15,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP15,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP15,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP15,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP15,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_QSFP16,  LED_MODE_QSFP_OFF,             ONLP_LED_MODE_OFF},
{LED_QSFP16,  LED_MODE_QSFP_YELLOW,             ONLP_LED_MODE_YELLOW},
{LED_QSFP16,  LED_MODE_QSFP_YELLOW_BLINKING,    ONLP_LED_MODE_YELLOW_BLINKING},
{LED_QSFP16,  LED_MODE_QSFP_GREEN,           ONLP_LED_MODE_GREEN},
{LED_QSFP16,  LED_MODE_QSFP_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},

{LED_CFP1,  LED_MODE_OFF,             ONLP_LED_MODE_OFF},
{LED_CFP1,  LED_MODE_PIU_BLUE,             ONLP_LED_MODE_BLUE},
{LED_CFP1,  LED_MODE_PIU_BLUE_BLINKING,    ONLP_LED_MODE_BLUE_BLINKING},
{LED_CFP1,  LED_MODE_PIU_GREEN,           ONLP_LED_MODE_GREEN},
{LED_CFP1,  LED_MODE_PIU_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_CFP2,  LED_MODE_OFF,             ONLP_LED_MODE_OFF},
{LED_CFP2,  LED_MODE_PIU_BLUE,             ONLP_LED_MODE_BLUE},
{LED_CFP2,  LED_MODE_PIU_BLUE_BLINKING,    ONLP_LED_MODE_BLUE_BLINKING},
{LED_CFP2,  LED_MODE_PIU_GREEN,           ONLP_LED_MODE_GREEN},
{LED_CFP2,  LED_MODE_PIU_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_CFP3,  LED_MODE_OFF,             ONLP_LED_MODE_OFF},
{LED_CFP3,  LED_MODE_PIU_BLUE,             ONLP_LED_MODE_BLUE},
{LED_CFP3,  LED_MODE_PIU_BLUE_BLINKING,    ONLP_LED_MODE_BLUE_BLINKING},
{LED_CFP3,  LED_MODE_PIU_GREEN,           ONLP_LED_MODE_GREEN},
{LED_CFP3,  LED_MODE_PIU_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},
{LED_CFP4,  LED_MODE_OFF,             ONLP_LED_MODE_OFF},
{LED_CFP4,  LED_MODE_PIU_BLUE,             ONLP_LED_MODE_BLUE},
{LED_CFP4,  LED_MODE_PIU_BLUE_BLINKING,    ONLP_LED_MODE_BLUE_BLINKING},
{LED_CFP4,  LED_MODE_PIU_GREEN,           ONLP_LED_MODE_GREEN},
{LED_CFP4,  LED_MODE_PIU_GREEN_BLINKING,  ONLP_LED_MODE_GREEN_BLINKING},

};

/* must map with onlp_led_id */
static char *leds[] = 
{
    NULL,
    "led_sys",
    "led_psu",
//    "led_sfp1",
//    "led_sfp2",
    "led_qsfp1",
    "led_qsfp2",
    "led_qsfp3",
    "led_qsfp4",
    "led_qsfp5",
    "led_qsfp6",
    "led_qsfp7",
    "led_qsfp8",
    "led_qsfp9",
    "led_qsfp10",
    "led_qsfp11",
    "led_qsfp12",
    "led_qsfp13",
    "led_qsfp14",
    "led_qsfp15",
    "led_qsfp16",
    "led_cfp1",
    "led_cfp2",
    "led_cfp3",
    "led_cfp4"    
};

/*
 * Get the information for the given LED OID.
 */
static onlp_led_info_t linfo[] =
{
    { }, /* Not used */
    {
        { ONLP_LED_ID_CREATE(LED_SYS), "Chassis LED 1 (SYSTEM LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_PSU), "Chassis LED 2 (PSU LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_RED | ONLP_LED_CAPS_RED_BLINKING | ONLP_LED_CAPS_GREEN,
    },
    /*
    {
        { ONLP_LED_ID_CREATE(LED_SFP1), "Chassis LED 3 (SFP1 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_SFP2), "Chassis LED 4 (SFP2 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    */
    {
        { ONLP_LED_ID_CREATE(LED_QSFP1), "QSFP28 LED 1", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP2), "QSFP28 LED 2", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP3), "QSFP28 LED 3", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP4), "QSFP28 LED 4", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP5), "QSFP28 LED 5", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP6), "QSFP28 LED 6", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP7), "QSFP28 LED 7", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP8), "QSFP28 LED 8", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP9), "QSFP28 LED 9", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP10), "QSFP28 LED 10", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP11), "QSFP28 LED 11", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP12), "QSFP28 LED 12", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP13), "QSFP28 LED 13", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP14), "QSFP28 LED 14", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP15), "QSFP28 LED 15", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_QSFP16), "QSFP28 LED 16", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_YELLOW | ONLP_LED_CAPS_YELLOW_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },

    {
        { ONLP_LED_ID_CREATE(LED_CFP1), "CFP2 LED 1 (CFP2-1 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_BLUE | ONLP_LED_CAPS_BLUE_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_CFP2), "CFP2 LED 2 (CFP2-2 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_BLUE | ONLP_LED_CAPS_BLUE_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_CFP3), "CFP2 LED 3 (CFP2-3 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_BLUE | ONLP_LED_CAPS_BLUE_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },
    {
        { ONLP_LED_ID_CREATE(LED_CFP4), "CFP2 LED 4 (CFP2-4 LED)", 0 },
        ONLP_LED_STATUS_PRESENT,
        ONLP_LED_CAPS_ON_OFF | ONLP_LED_CAPS_BLUE | ONLP_LED_CAPS_BLUE_BLINKING | ONLP_LED_CAPS_GREEN | ONLP_LED_CAPS_GREEN_BLINKING,
    },

    
};
/*
static int driver_to_onlp_led_mode(enum onlp_led_id id, enum led_light_mode driver_led_mode)
{
    int i, nsize = sizeof(led_map)/sizeof(led_map[0]);
    
    for (i = 0; i < nsize; i++)
    {
        if (id == led_map[i].id && driver_led_mode == led_map[i].driver_led_mode)
        {
            return led_map[i].onlp_led_mode;
        }
    }
    
    return 0;
}

static int onlp_to_driver_led_mode(enum onlp_led_id id, onlp_led_mode_t onlp_led_mode)
{
    int i, nsize = sizeof(led_map)/sizeof(led_map[0]);
    
    for(i = 0; i < nsize; i++)
    {
        if (id == led_map[i].id && onlp_led_mode == led_map[i].onlp_led_mode)
        {
            return led_map[i].driver_led_mode;
        }
    }
    
    return 0;
}
*/
int
turn_on_phy_led(uint32_t addr, uint32_t reg, uint16_t bit)
{
    uint16_t value=0;

    onlp_mdio_read_c22(addr, reg, &value);
    value = value | (0xf << bit);
    return onlp_mdio_write_c22(addr, reg, value);
    return value;
}

int
turn_off_phy_led(uint32_t addr, uint32_t reg, uint16_t bit)
{
    uint16_t value=0;

    onlp_mdio_read_c22(addr, reg, &value);
    value = value | (0xe << bit);
    value = value & ~(0x1 << bit);
    return onlp_mdio_write_c22(addr, reg, value);
    //return value;
}


/*
 * This function will be called prior to any other onlp_ledi_* functions.
 */
int
onlp_ledi_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_ledi_info_get(onlp_oid_t id, onlp_led_info_t* info)
{
    int  lid, mode, value;
    //int  value1, value2;
    //uint32_t    addr;
    //uint16_t	value16 = 0;

    VALIDATE(id);
	
    lid = ONLP_OID_ID_GET(id);

    /* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo[ONLP_OID_ID_GET(id)];

    /* Get LED mode */
    switch (lid) {
	/*
        case LED_SFP1:
        case LED_SFP2:
            addr = (lid == LED_SFP1) ? SFP1_PHYADDRESS : SFP2_PHYADDRESS;

	    onlp_mdio_read_c22(addr, SFP_LED_REGISTER, &value16);
	    value1 = 0xf << SFP_LED_BIT_GREEN;
	    value2 = 0xf << SFP_LED_BIT_YELLOW;

            if ( (value16 & value1) == value1 )
            {
                mode = LED_MODE_SFP_GREEN;
	    }		
	    else if ( (value16 & value2) == value2 )
            {
                mode = LED_MODE_SFP_YELLOW;
            }
	    else
	    {
                mode = LED_MODE_OFF;
	    }
            break;
        */

	default:
            /* Get LED mode */
            if (lid > LED_CFP4)
            {
                DEBUG_PRINT("Unable to read status from file " LED_FORMAT);
		mode = LED_MODE_OFF;
                return ONLP_STATUS_E_INTERNAL;
            }
				    
            if ((onlp_file_read_int(&value, LED_FORMAT, leds[lid]) < 0))
            {
                DEBUG_PRINT("Unable to read status from file " LED_FORMAT, leds[lid]);
                return ONLP_STATUS_E_INTERNAL;
            }

            mode = value;
	    break;
    }

    info->mode = mode;//driver_to_onlp_led_mode(lid, mode);

    return ONLP_STATUS_OK;
}

/*
 * Turn an LED on or off.
 *
 * This function will only be called if the LED OID supports the ONOFF
 * capability.
 *
 * What 'on' means in terms of colors or modes for multimode LEDs is
 * up to the platform to decide. This is intended as baseline toggle mechanism.
 * Wistron : Currenlty we dont see any use-case for ON and also the color is not known
 * and hence is not supported.
 */
int
onlp_ledi_set(onlp_oid_t id, int on_or_off)
{
    VALIDATE(id);

    if (!on_or_off) {
        return onlp_ledi_mode_set(id, ONLP_LED_MODE_OFF);
    }

    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function puts the LED into the given mode. It is a more functional
 * interface for multimode LEDs.
 *
 * Only modes reported in the LED's capabilities will be attempted.
 */
int
onlp_ledi_mode_set(onlp_oid_t id, onlp_led_mode_t onlp_mode)
{
    int  lid;//, _value;
    //uint32_t addr;
    //int  rv = ONLP_STATUS_OK;    
    VALIDATE(id);

    lid = ONLP_OID_ID_GET(id);

    if (lid > LED_CFP4)
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    if (onlp_file_write_int(onlp_mode, LED_FORMAT, leds[lid]) < 0)
    {
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

