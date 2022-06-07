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
#include <onlp/platformi/psui.h>
#include <onlplib/file.h>
#include "platform_lib.h"

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_PSU(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)

int
onlp_psui_init(void)
{
    return ONLP_STATUS_OK;
}

/*
 * Get all information about the given PSU oid.
 */
static onlp_psu_info_t pinfo[] =
{
    { }, /* Not used */
    {
        { ONLP_PSU_ID_CREATE(PSU1_ID), "PSU-1", 0 },
    },
    {
        { ONLP_PSU_ID_CREATE(PSU2_ID), "PSU-2", 0 },
    }
};

int
onlp_psui_status_get(onlp_oid_t id, uint32_t* rv)
{
    int idx = ONLP_OID_ID_GET(id);
    int ret = get_psu_presence(idx);
    if ( ret < 0 ) {
        return -1;
    } else if ( ret == 0 ) {
        *rv = ONLP_STATUS_E_MISSING;
    } else {
        *rv = ONLP_STATUS_OK;
    }
    return ONLP_STATUS_OK;
}

int
onlp_psui_info_get(onlp_oid_t id, onlp_psu_info_t* info)
{
    int val   = 0;
    int ret   = ONLP_STATUS_OK;
    int index = ONLP_OID_ID_GET(id);
    uint32_t status;

    VALIDATE(id);

    memset(info, 0, sizeof(onlp_psu_info_t));
    *info = pinfo[index]; /* Set the onlp_oid_hdr_t */

    if ( onlp_psui_status_get(id, &status) == ONLP_STATUS_OK && status == ONLP_STATUS_E_MISSING ) {
        info->status |= ONLP_PSU_STATUS_UNPLUGGED;
        return ONLP_STATUS_OK;
    }

    /* Get the present state */
    ret = onlp_file_read_int(&val, "%s""psu%d_status", PSU_SYSFS_PATH, index);
    if (ret < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s""psu%d_present)\r\n", PSU_SYSFS_PATH, index);
        return ONLP_STATUS_E_INTERNAL;
    }

    if (val & 0x01) {
        info->status |= ONLP_PSU_STATUS_PRESENT;
    }
    if (val & 0x02) {
        info->status |= ONLP_PSU_STATUS_FAILED;
    }
    if (val & 0x08) {
        info->status |= ONLP_PSU_STATUS_UNPLUGGED;
    }

    /* Get the PSU pout */
    ret = onlp_file_read_int(&val, "%s""psu%d_pout", PSU_SYSFS_PATH, index);
    if (ret == 0) {
        info->caps |= ONLP_PSU_CAPS_POUT;
        info->mpout = val; /* values read from the file are in milli */
    }

    /* Get the PSU pin */
    ret = onlp_file_read_int(&val, "%s""psu%d_pin", PSU_SYSFS_PATH, index);
    if (ret == 0) {
        info->caps |= ONLP_PSU_CAPS_PIN;
        info->mpin = val; /* values read from the file are in milli */
    }

    /* Get the PSU vout */
    ret = onlp_file_read_int(&val, "%s""psu%d_vout", PSU_SYSFS_PATH, index);
    if (ret == 0) {
        info->caps |= ONLP_PSU_CAPS_VOUT;
        info->mvout = val; /* values read from the file are in milli */
    }

    /* Get the PSU vin */
    ret = onlp_file_read_int(&val, "%s""psu%d_vin", PSU_SYSFS_PATH, index);
    if (ret == 0) {
        info->caps |= ONLP_PSU_CAPS_VIN;
        info->mvin = val; /* values read from the file are in milli */
    }

    /* Get the PSU iout */
    ret = onlp_file_read_int(&val, "%s""psu%d_iout", PSU_SYSFS_PATH, index);
    if (ret == 0) {
        info->caps |= ONLP_PSU_CAPS_IOUT;
        info->miout = val; /* values read from the file are in milli*/
    }

    /* Get the PSU iin */
    ret = onlp_file_read_int(&val, "%s""psu%d_iin", PSU_SYSFS_PATH, index);
    if (ret == 0) {
        info->caps |= ONLP_PSU_CAPS_IIN;
        info->miin = val; /* values read from the file are in milli */
    }

    /* Read model */
    char *string = NULL;

    int len = onlp_file_read_str(&string, "%s""psu%d_model", PSU_SYSFS_PATH, index);
    if (string && len) {
        aim_strlcpy(info->model, string, len);
        aim_free(string);
    } else {
        // when model is empty, use type field instead
        len = onlp_file_read_str(&string, "%s""psu%d_type", PSU_SYSFS_PATH, index);
        if (string && len) {
            aim_strlcpy(info->model, string, len);
            aim_free(string);
        }
    }

    /* Read serial */
    len = onlp_file_read_str(&string, "%s""psu%d_serial", PSU_SYSFS_PATH, index);
    if (string && len) {
        aim_strlcpy(info->serial, string, len);
        aim_free(string);
    }

  /* Set the associated oid_table */
    val = 0;
    if (onlp_file_read_int(&val, "%s""psu%d_fan_rpm", PSU_SYSFS_PATH, index) == 0 && val) {
        info->hdr.coids[0] = ONLP_FAN_ID_CREATE(index + CHASSIS_FAN_COUNT);
    }

    val = 0;
    if (onlp_file_read_int(&val, "%s""psu%d_temp", PSU_SYSFS_PATH, index) == 0 && val) {
        info->hdr.coids[1] = ONLP_THERMAL_ID_CREATE(index + CHASSIS_THERMAL_COUNT);
    }

    return ONLP_STATUS_OK;
}

int
onlp_psui_ioctl(onlp_oid_t pid, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

