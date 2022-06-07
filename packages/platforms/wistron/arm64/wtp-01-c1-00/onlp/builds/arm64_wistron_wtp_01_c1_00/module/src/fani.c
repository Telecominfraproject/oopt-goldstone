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
 * Fan Platform Implementation Defaults.
 *
 ***********************************************************/
#include <onlplib/file.h>
#include <onlp/platformi/fani.h>
#include "platform_lib.h"
#include <unistd.h>

/* Average of front and rear max speeds */
#define MAX_FAN_SPEED		((24200 + 22000)/2)
/* Need to check the PSU data-sheet for the exact value */
#define MAX_PSU_FAN_SPEED 	18000

#define PSU_FAN_INFO(pid, fid) 		\
    { \
        { ONLP_FAN_ID_CREATE(FAN##fid##_ON_PSU##pid), "PSU "#pid" - Fan "#fid, ONLP_PSU_ID_CREATE(pid) },\
        0x0,\
        ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE | ONLP_FAN_CAPS_SET_PERCENTAGE,\
        0,\
        0,\
        ONLP_FAN_MODE_INVALID,\
    }

/* Static fan information */
onlp_fan_info_t finfo[] = {
    { }, /* Not used */
	PSU_FAN_INFO(1, 1),
	PSU_FAN_INFO(1, 2),
	PSU_FAN_INFO(1, 3),
	PSU_FAN_INFO(2, 1),
	PSU_FAN_INFO(2, 2),
	PSU_FAN_INFO(2, 3)
};

#define VALIDATE(_id)                           \
    do {                                        \
        if(!ONLP_OID_IS_FAN(_id)) {             \
            return ONLP_STATUS_E_INVALID;       \
        }                                       \
    } while(0)
 /*
static int
_onlp_fani_info_get_fan_on_psu(int pid, int fid, onlp_fan_info_t* info)
{
    int ret   = ONLP_STATUS_OK;
    uint8_t     buffer[9];
    int size=8, flags=1;
    uint32_t status;

    // Get the PSU present state 
    ret = onlp_file_read_int(&val, "%s""psu%d_status", PSU_SYSFS_PATH, index);
    if (ret < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s""psu%d_status)\r\n", PSU_SYSFS_PATH, index);
	return ONLP_STATUS_E_INTERNAL;
    } else if (val == 0) {
        info->status |= ONLP_FAN_STATUS_FAILED;

        return ONLP_STATUS_OK;
    }

    info->percentage = buffer[0];
    info->status |= ONLP_FAN_STATUS_PRESENT;

    // get fan RPM
    flags = 2;

    ret = onlp_can_read(pid, size, buffer, flags);
    if (ret < 0) {
        AIM_LOG_ERROR("Unable to read fan status from psu[%d][%x]\r\n", pid, ret);
        
	SEM_UNLOCK;
	return ONLP_STATUS_OK;
	//return ONLP_STATUS_E_INTERNAL;
    }

    // get fan speed 
    switch (fid) {
        case 1:
	    info->rpm = ((buffer[1] << 8) | buffer[0]);
	    break;

	case 2:
            info->rpm = ((buffer[3] << 8) | buffer[2]);
            break;

        case 3:
            info->rpm = ((buffer[5] << 8) | buffer[4]);
            break;
    }

    info->status |= (info->rpm == 0) ? ONLP_FAN_STATUS_FAILED : 0;

    SEM_UNLOCK;
    
    return ONLP_STATUS_OK;
}
*/

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int
onlp_fani_init(void)
{
    return ONLP_STATUS_OK;
}

int
onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info)
{
    int val   = 0;
    int ret   = ONLP_STATUS_OK;
    int fid = ONLP_OID_ID_GET(id);
    int pid = PSU1_ID;
    //uint32_t status;

    VALIDATE(id);

    memset(info, 0, sizeof(onlp_fan_info_t));

    switch (fid)
    {
        case FAN1_ON_PSU1:
	case FAN2_ON_PSU1:
	case FAN3_ON_PSU1:
            /* Get the present state */
	    pid = PSU1_ID;

	    *info = finfo[(pid-1)*3+fid]; /* Set the onlp_oid_hdr_t */
    
	    /* Get the PSU present state */
            ret = onlp_file_read_int(&val, "%s""psu%d_status", PSU_SYSFS_PATH, pid);
            if (ret < 0) {
                AIM_LOG_ERROR("Unable to read status from (%s""psu%d_status)\r\n", PSU_SYSFS_PATH, pid);

                return ONLP_STATUS_E_INTERNAL;
            } else if (val == 0) {
                info->status |= ONLP_FAN_STATUS_FAILED;

                return ONLP_STATUS_OK;
            }

            info->status |= ONLP_FAN_STATUS_PRESENT;

            /* Get the FAN RPM */
            ret = onlp_file_read_int(&val, "%s""psu%d_fan%d_rpm", PSU_SYSFS_PATH, pid, fid);
            if (ret == 0) {
                info->rpm = val; /* values read from the file are in milli */
            }

            /* Get the FAN PWM */
            ret = onlp_file_read_int(&val, "%s""psu%d_fan_pwm", PSU_SYSFS_PATH, pid);
            if (ret == 0) {
                info->percentage = val; /* values read from the file are in milli */
            }

            break;
	case FAN1_ON_PSU2:
	case FAN2_ON_PSU2:
	case FAN3_ON_PSU2:
            /* Get the present state */
            pid = PSU2_ID;

	    *info = finfo[(pid-1)*3+fid-FAN1_ON_PSU2+1]; /* Set the onlp_oid_hdr_t */

            /* Get the PSU present state */
            ret = onlp_file_read_int(&val, "%s""psu%d_status", PSU_SYSFS_PATH, pid);
            if (ret < 0) {
                AIM_LOG_ERROR("Unable to read status from (%s""psu%d_status)\r\n", PSU_SYSFS_PATH, pid);

                return ONLP_STATUS_E_INTERNAL;
            } else if (val == 0) {
                info->status |= ONLP_FAN_STATUS_FAILED;

                return ONLP_STATUS_OK;
            }

            info->status |= ONLP_FAN_STATUS_PRESENT;

            /* Get the FAN RPM */
            ret = onlp_file_read_int(&val, "%s""psu%d_fan%d_rpm", PSU_SYSFS_PATH, pid, (fid-FAN1_ON_PSU2+1));
            if (ret == 0) {
                info->rpm = val; /* values read from the file are in milli */
            }

            /* Get the FAN PWM */
            ret = onlp_file_read_int(&val, "%s""psu%d_fan_pwm", PSU_SYSFS_PATH, pid);
            if (ret == 0) {
                info->percentage = val; /* values read from the file are in milli */
            }

            break;
        default:
            ret = ONLP_STATUS_E_INVALID;
            break;
    }	
    
    return ret;
}

/*
 * This function sets the speed of the given fan in RPM.
 *
 * This function will only be called if the fan supprots the RPM_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_rpm_set(onlp_oid_t id, int rpm)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan speed of the given OID as a percentage.
 *
 * This will only be called if the OID has the PERCENTAGE_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int
onlp_fani_percentage_set(onlp_oid_t id, int p)
{
    int fid, pid=0, val=0;
    int ret = ONLP_STATUS_OK;

    VALIDATE(id);

    fid = ONLP_OID_ID_GET(id);

    /* reject p=0 (p=0, stop fan) */
    if (p == 0) {
        return ONLP_STATUS_E_INVALID;
    }

    switch (fid)
    {
        case FAN1_ON_PSU1:
        case FAN2_ON_PSU1:
        case FAN3_ON_PSU1:
            pid = PSU1_ID;
            break;
        case FAN1_ON_PSU2:
        case FAN2_ON_PSU2:
        case FAN3_ON_PSU2:
            pid = PSU2_ID;
            break;
        default:
            ret = ONLP_STATUS_E_INVALID;
	    goto exit;
    }

    /* Get the PSU present state */
    ret = onlp_file_read_int(&val, "%s""psu%d_status", PSU_SYSFS_PATH, pid);
    if (ret < 0) {
        AIM_LOG_ERROR("Unable to read status from (%s""psu%d_status), ret[%x]\r\n", PSU_SYSFS_PATH, pid, ret);

        ret = ONLP_STATUS_E_INTERNAL;
	goto exit;
    } else if (val == 0) {
        // PIU not present
        ret = ONLP_STATUS_OK;
	goto exit;
    }

    if (onlp_file_write_int(p,
                          "%s"
                          "psu%d_fan_pwm",
                          PSU_SYSFS_PATH, pid) < 0) {
        AIM_LOG_ERROR(
            "Unable to write data to file %s"
            "psu%d_fan_pwm",
            PSU_SYSFS_PATH, pid);
        return ONLP_STATUS_E_INTERNAL;
    }

exit:

    return ret;
}


/*
 * This function sets the fan speed of the given OID as per
 * the predefined ONLP fan speed modes: off, slow, normal, fast, max.
 *
 * Interpretation of these modes is up to the platform.
 *
 */
int
onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * This function sets the fan direction of the given OID.
 *
 * This function is only relevant if the fan OID supports both direction
 * capabilities.
 *
 * This function is optional unless the functionality is available.
 */
int
onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Generic fan ioctl. Optional.
 */
int
onlp_fani_ioctl(onlp_oid_t id, va_list vargs)
{
    return ONLP_STATUS_E_UNSUPPORTED;
}

