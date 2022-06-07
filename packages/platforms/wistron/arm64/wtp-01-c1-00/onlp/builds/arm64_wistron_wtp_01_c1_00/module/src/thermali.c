/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2014 Accton Technology Corporation.
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
 * Thermal Sensor Platform Implementation.
 *
 ***********************************************************/
#include <onlplib/file.h>
#include <onlp/platformi/thermali.h>
#include "platform_lib.h"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_THERMAL(_id)) {         \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)
/*
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
*/

static char* devfiles__[] =  /* must map with onlp_thermal_id */
{
    NULL,
    "/sys/bus/i2c/devices/0-004c/hwmon/hwmon3/temp1_input",
    "/sys/bus/i2c/devices/0-004c/hwmon/hwmon3/temp2_input",
    "/sys/bus/i2c/devices/0-0048/hwmon/hwmon0/temp1_input",
    "/sys/bus/i2c/devices/0-0049/hwmon/hwmon1/temp1_input",
    "/sys/bus/i2c/devices/0-004a/hwmon/hwmon2/temp1_input",
    "/sys/bus/i2c/devices/0-0018/hwmon/hwmon4/temp1_input",
    "/sys/bus/i2c/devices/0-0018/hwmon/hwmon4/temp2_input",
    "/sys/bus/i2c/devices/0-0029/hwmon/hwmon5/temp1_input",
    "/sys/bus/i2c/devices/0-0029/hwmon/hwmon5/temp2_input",
};


/* Static values */
static onlp_thermal_info_t linfo[] = {
    { }, /* Not used */
    { { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_SA56004), "SA56004 1", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_SA56004), "SA56004 2", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_TMP75), "TMP75 1", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_TMP75), "TMP75 2", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_TMP75), "TMP75 3", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_LM95245), "LM95245 1", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_LM95245), "LM95245 2", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_LM95245), "LM95245 3", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
    { { ONLP_THERMAL_ID_CREATE(THERMAL_4_ON_LM95245), "LM95245 4", 0},
        ONLP_THERMAL_STATUS_PRESENT,
        ONLP_THERMAL_CAPS_ALL, 0, ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS
    },
};

// Warning / error / shutdown temperature for each sensor, +2 for M.2 SSD & QSFP28
// EA Team update 5/6/2022
static int tcaution[CHASSIS_THERMAL_COUNT+2][3] = { {80000, 82000, 84000},   // SA56004
                                            {80000, 82000, 84000},   // SA56004
                                            {60000, 62000, 65000},   // U37-tmp75-1
                                            {80000, 82000, 84000},   // U38-tmp75-1
                                            {80000, 82000, 84000},   // U39-tmp75-1
                                            {80000, 82000, 84000},   // U30-Im925245
                                            {80000, 82000, 84000},   // U30-Im925245
                                            {80000, 82000, 84000},   // U30-Im925245
                                            {80000, 82000, 84000},   // U30-Im925245
                                            {85000, 90000, 95000},   // M.2 SSD
                                            {90000, 100000, 105000}};   // CFP2

// 3/17/2022
/*
static int tcaution[CHASSIS_THERMAL_COUNT+2][3] = { {85000, 87000, 88000}, 
				            {85000, 87000, 88000},
				            {60000, 62000, 65000},
				            {90000, 92000, 94000},
				            {90000, 92000, 94000},
				            {110000, 112000, 114000},
				            {110000, 112000, 114000},
				            {110000, 112000, 114000},
				            {110000, 112000, 114000},
				            {85000, 90000, 95000},
				            {90000, 100000, 105000}};
*/

/*
 * This will be called to intiialize the thermali subsystem.
 */
int
onlp_thermali_init(void)
{
    return ONLP_STATUS_OK;
}

/*
 * Retrieve the information structure for the given thermal OID.
 *
 * If the OID is invalid, return ONLP_E_STATUS_INVALID.
 * If an unexpected error occurs, return ONLP_E_STATUS_INTERNAL.
 * Otherwise, return ONLP_STATUS_OK with the OID's information.
 *
 * Note -- it is expected that you fill out the information
 * structure even if the sensor described by the OID is not present.
 */
int
onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info)
{
    int rt;
    int local_id, index;
    VALIDATE(id);

    local_id = ONLP_OID_ID_GET(id);

    /* Set the onlp_oid_hdr_t and capabilities */
    *info = linfo[local_id];

    rt = onlp_file_read_int(&info->mcelsius, devfiles__[local_id]);

    // local_id start from 1
    index = local_id-1;
    switch (index) 
    {
	case 0:	// THERMAL_1_ON_SA56004
	case 1: // THERMAL_2_ON_SA56004
	case 2: // THERMAL_1_ON_TMP75
	case 3: // THERMAL_2_ON_TMP75
	case 4: // THERMAL_3_ON_TMP75
	case 5: // THERMAL1_ON_LM95245
	case 6: // THERMAL2_ON_LM95245
	case 7: // THERMAL3_ON_LM95245
	case 8: // THERMAL4_ON_LM95245
		info->thresholds.warning = tcaution[index][0];
		info->thresholds.error = tcaution[index][1];
		info->thresholds.shutdown = tcaution[index][2];
		break;
	case 9: // M.2 SSD
		info->thresholds.warning = tcaution[index][0];
		info->thresholds.error = tcaution[index][1];
		info->thresholds.shutdown = tcaution[index][2];
	default: // QSFP28
		info->thresholds.warning = tcaution[CHASSIS_THERMAL_COUNT+1][0];
		info->thresholds.error = tcaution[CHASSIS_THERMAL_COUNT+1][1];
		info->thresholds.shutdown = tcaution[CHASSIS_THERMAL_COUNT+1][2];
		break;
    }

    return rt;
}
