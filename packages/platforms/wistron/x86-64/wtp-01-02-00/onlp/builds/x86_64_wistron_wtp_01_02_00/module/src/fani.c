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
#include <onlp/platformi/fani.h>
#include <onlplib/file.h>

#include "platform_lib.h"

enum fan_id {
  FAN_1_ON_FAN_BOARD = 1,
  FAN_2_ON_FAN_BOARD,
  FAN_3_ON_FAN_BOARD,
  FAN_4_ON_FAN_BOARD,
  FAN_5_ON_FAN_BOARD,
  FAN_6_ON_FAN_BOARD,
  FAN_7_ON_FAN_BOARD,
  FAN_8_ON_FAN_BOARD,
  FAN_9_ON_FAN_BOARD,
  FAN_10_ON_FAN_BOARD,
  FAN_1_ON_PSU_1,
  FAN_1_ON_PSU_2
};

/* Average of front and rear max speeds */
#define MAX_FAN_SPEED ((24200 + 22000) / 2)
/* Need to check the PSU data-sheet for the exact value */
#define MAX_PSU_FAN_SPEED 18000

#define CHASSIS_FAN_INFO(fid, desc)                               \
  {                                                               \
    {ONLP_FAN_ID_CREATE(FAN_##fid##_ON_FAN_BOARD), desc, 0}, 0x0, \
        ONLP_FAN_CAPS_SET_PERCENTAGE | ONLP_FAN_CAPS_GET_RPM |    \
            ONLP_FAN_CAPS_GET_PERCENTAGE,                         \
        0, 0, ONLP_FAN_MODE_INVALID, "DFPK0456B2SY037",           \
  }

#define PSU_FAN_INFO(pid, fid)                                                 \
  {                                                                            \
    {ONLP_FAN_ID_CREATE(FAN_##fid##_ON_PSU_##pid), "PSU " #pid " - Fan " #fid, \
     ONLP_PSU_ID_CREATE(pid)},                                                 \
        0x0, ONLP_FAN_CAPS_GET_RPM | ONLP_FAN_CAPS_GET_PERCENTAGE, 0, 0,       \
        ONLP_FAN_MODE_INVALID,                                                 \
  }

/* Static fan information */
onlp_fan_info_t finfo[] = {{}, /* Not used */
                           CHASSIS_FAN_INFO(1, "Chassis-Fan-1 [Front]"),
                           CHASSIS_FAN_INFO(2, "Chassis-Fan-1 [Rear]"),
                           CHASSIS_FAN_INFO(3, "Chassis-Fan-2 [Front]"),
                           CHASSIS_FAN_INFO(4, "Chassis-Fan-2 [Rear]"),
                           CHASSIS_FAN_INFO(5, "Chassis-Fan-3 [Front]"),
                           CHASSIS_FAN_INFO(6, "Chassis-Fan-3 [Rear]"),
                           CHASSIS_FAN_INFO(7, "Chassis-Fan-4 [Front]"),
                           CHASSIS_FAN_INFO(8, "Chassis-Fan-4 [Rear]"),
                           CHASSIS_FAN_INFO(9, "Chassis-Fan-5 [Front]"),
                           CHASSIS_FAN_INFO(10, "Chassis-Fan-5 [Rear]"),
                           PSU_FAN_INFO(1, 1),
                           PSU_FAN_INFO(2, 1)};

#define VALIDATE(_id)               \
  do {                              \
    if (!ONLP_OID_IS_FAN(_id)) {    \
      return ONLP_STATUS_E_INVALID; \
    }                               \
  } while (0)

static int _onlp_fani_info_get_fan(int fid, onlp_fan_info_t* info) {
  int value;
  int ret = ONLP_STATUS_OK;

  /* get fan status */
  ret = onlp_file_read_int(&value,
                           "%s"
                           "fan%d_status",
                           FAN_SYSFS_PATH, fid);
  if (ret < 0) {
    AIM_LOG_ERROR(
        "Unable to read status from (%s"
        "fan%d_status)",
        FAN_SYSFS_PATH, fid);
    return ONLP_STATUS_E_INTERNAL;
  }

  if (value != 0) {
    if (value & 0x1F) {
      /* Currenly the status is set to failed for Non-recoverble,
       * critial, non-critical conditions */
      info->status |= ONLP_FAN_STATUS_FAILED;
    }
  } else {
    info->status |= ONLP_FAN_STATUS_FAILED;
  }

  /* Airflow direction is front->back as mentioned in HW specification */
  info->status |= ONLP_FAN_STATUS_F2B;

  /* get fan speed*/
  ret = onlp_file_read_int(&value,
                           "%s"
                           "fan%d_rpm",
                           FAN_SYSFS_PATH, fid);
  if (ret < 0) {
    AIM_LOG_ERROR(
        "Unable to read status from (%s"
        "fan%d_rpm)",
        FAN_SYSFS_PATH, fid);
    return ONLP_STATUS_E_INTERNAL;
  }

  if (value != 0) {
    info->rpm = value * 100;
    info->status |= ONLP_FAN_STATUS_PRESENT;
  } else {
    info->status |= ONLP_FAN_STATUS_FAILED;
    info->rpm = 0;
    /* For the rear-fan , check the front-fan speed to determine the presence*/
    if (fid % 2 == 0) {
      ret = onlp_file_read_int(&value,
                               "%s"
                               "fan%d_rpm",
                               FAN_SYSFS_PATH, fid - 1);
      if ((ret == 0) && (value == 0)) info->status &= ~ONLP_FAN_STATUS_PRESENT;
    } else {
      /* For the front-fan , check the rear-fan speed to determine the
       * presence*/
      ret = onlp_file_read_int(&value,
                               "%s"
                               "fan%d_rpm",
                               FAN_SYSFS_PATH, fid + 1);
      if ((ret == 0) && (value == 0)) info->status &= ~ONLP_FAN_STATUS_PRESENT;
    }
  }

  /* Get speed percentage from rpm */
  info->percentage = (info->rpm * 100) / MAX_FAN_SPEED;

  return ONLP_STATUS_OK;
}

static int _onlp_fani_info_get_fan_on_psu(int pid, onlp_fan_info_t* info) {
  int val = 0, ret = 0;

  /* Check how to get the presence of fan on PSU,
   * Currenly assuming that fan cannot be detached from PSU,
   * PSU  presence = PSU-FAN presence */
  ret = onlp_file_read_int(&val,
                           "%s"
                           "psu%d_status",
                           PSU_SYSFS_PATH, pid);
  if (ret < 0) {
    AIM_LOG_ERROR(
        "Unable to read status from (%s"
        "psu%d_present)\r\n",
        PSU_SYSFS_PATH, pid);
    return ONLP_STATUS_E_INTERNAL;
  }

  if (val & 0x01) {
    info->status |= ONLP_FAN_STATUS_PRESENT;
  }

  /* get fan direction
   */
  info->status |= ONLP_FAN_STATUS_F2B;

  /* get fan speed */
  if (onlp_file_read_int(&val,
                         "%s"
                         "psu%d_fan_rpm",
                         PSU_SYSFS_PATH, pid) == 0 &&
      val) {
    info->rpm = val;
    info->percentage = (info->rpm * 100) / MAX_PSU_FAN_SPEED;
    info->status |= (val == 0) ? ONLP_FAN_STATUS_FAILED : 0;
  }

  return ONLP_STATUS_OK;
}

/*
 * This function will be called prior to all of onlp_fani_* functions.
 */
int onlp_fani_init(void) { return ONLP_STATUS_OK; }

int onlp_fani_info_get(onlp_oid_t id, onlp_fan_info_t* info) {
  int rc = 0;
  int fid;
  VALIDATE(id);

  fid = ONLP_OID_ID_GET(id);
  *info = finfo[fid];

  switch (fid) {
    case FAN_1_ON_PSU_1:
      rc = _onlp_fani_info_get_fan_on_psu(PSU1_ID, info);
      break;
    case FAN_1_ON_PSU_2:
      rc = _onlp_fani_info_get_fan_on_psu(PSU2_ID, info);
      break;
    case FAN_1_ON_FAN_BOARD:
    case FAN_2_ON_FAN_BOARD:
    case FAN_3_ON_FAN_BOARD:
    case FAN_4_ON_FAN_BOARD:
    case FAN_5_ON_FAN_BOARD:
    case FAN_6_ON_FAN_BOARD:
    case FAN_7_ON_FAN_BOARD:
    case FAN_8_ON_FAN_BOARD:
    case FAN_9_ON_FAN_BOARD:
    case FAN_10_ON_FAN_BOARD:
      rc = _onlp_fani_info_get_fan(fid, info);
      break;
    default:
      rc = ONLP_STATUS_E_INVALID;
      break;
  }

  return rc;
}

/*
 * This function sets the speed of the given fan in RPM.
 *
 * This function will only be called if the fan supprots the RPM_SET
 * capability.
 *
 * It is optional if you have no fans at all with this feature.
 */
int onlp_fani_rpm_set(onlp_oid_t id, int rpm) {
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
int onlp_fani_percentage_set(onlp_oid_t id, int p) {
  int fid;

  VALIDATE(id);

  fid = ONLP_OID_ID_GET(id);

  /* reject p=0 (p=0, stop fan) */
  if (p == 0) {
    return ONLP_STATUS_E_INVALID;
  }

  if (fid < FAN_1_ON_FAN_BOARD || fid > FAN_5_ON_FAN_BOARD) {
    return ONLP_STATUS_E_INVALID;
  }

  if (onlp_file_write_int(p,
                          "%s"
                          "fan%d_pwm",
                          FAN_SYSFS_PATH, fid) < 0) {
    AIM_LOG_ERROR(
        "Unable to write data to file %s"
        "fan%d_pwm",
        FAN_SYSFS_PATH, fid);
    return ONLP_STATUS_E_INTERNAL;
  }

  return ONLP_STATUS_OK;
}

/*
 * This function sets the fan speed of the given OID as per
 * the predefined ONLP fan speed modes: off, slow, normal, fast, max.
 *
 * Interpretation of these modes is up to the platform.
 *
 */
int onlp_fani_mode_set(onlp_oid_t id, onlp_fan_mode_t mode) {
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
int onlp_fani_dir_set(onlp_oid_t id, onlp_fan_dir_t dir) {
  return ONLP_STATUS_E_UNSUPPORTED;
}

/*
 * Generic fan ioctl. Optional.
 */
int onlp_fani_ioctl(onlp_oid_t id, va_list vargs) {
  return ONLP_STATUS_E_UNSUPPORTED;
}
