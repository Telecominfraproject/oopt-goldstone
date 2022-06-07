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
#include <onlp/platformi/thermali.h>
#include <onlplib/file.h>

#include "platform_lib.h"

#define VALIDATE(_id)                \
  do {                               \
    if (!ONLP_OID_IS_THERMAL(_id)) { \
      return ONLP_STATUS_E_INVALID;  \
    }                                \
  } while (0)

static char* ipmi_devfiles__[] = /* must map with onlp_thermal_id */
    {
        NULL,
        "/sys/devices/platform/wtp_01_02_00_thermal/temp1_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp2_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp3_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp4_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp5_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp6_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp7_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp8_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp9_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp10_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp11_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp12_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp13_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp14_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp15_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp16_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp17_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp18_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp19_input",
        "/sys/devices/platform/wtp_01_02_00_thermal/temp20_input",
        "/sys/devices/platform/wtp_01_02_00_psu/psu1_temp",
        "/sys/devices/platform/wtp_01_02_00_psu/psu2_temp",
};

#if 0
static char* cpu_coretemp_files[] =
{
    NULL,
};
#endif

/* Static values */
static onlp_thermal_info_t linfo[] = {
    {}, /* Not used */
    {{ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_MAIN_BROAD), "Ambient Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_MAIN_BROAD), "Switch Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_MAIN_BROAD), "Sw Outlet Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_4_ON_MAIN_BROAD), "Sw Inlet Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_5_ON_MAIN_BROAD), "Sw Zone Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_6_ON_MAIN_BROAD), "CPU Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_7_ON_MAIN_BROAD), "CPU Inlet Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_8_ON_MAIN_BROAD), "DIMM Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_9_ON_MAIN_BROAD), "VR Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_10_ON_MAIN_BROAD), "PIU DSP1 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_11_ON_MAIN_BROAD), "PIU DSP2 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_12_ON_MAIN_BROAD), "PIU DSP3 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_13_ON_MAIN_BROAD), "PIU DSP4 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_14_ON_MAIN_BROAD), "M.2 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_15_ON_MAIN_BROAD), "PSU1 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_16_ON_MAIN_BROAD), "PSU2 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_17_ON_MAIN_BROAD), "ACO1 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_18_ON_MAIN_BROAD), "ACO2 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_19_ON_MAIN_BROAD), "ACO3 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_20_ON_MAIN_BROAD), "ACO4 Temp", 0},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_PSU1), "PSU-1 Thermal Sensor 1",
      ONLP_PSU_ID_CREATE(PSU1_ID)},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS},
    {{ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_PSU2), "PSU-2 Thermal Sensor 1",
      ONLP_PSU_ID_CREATE(PSU2_ID)},
     ONLP_THERMAL_STATUS_PRESENT,
     ONLP_THERMAL_CAPS_ALL,
     0,
     ONLP_THERMAL_THRESHOLD_INIT_DEFAULTS}};

/*
 * This will be called to intiialize the thermali subsystem.
 */
int onlp_thermali_init(void) { return ONLP_STATUS_OK; }

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
int onlp_thermali_info_get(onlp_oid_t id, onlp_thermal_info_t* info) {
  int tid, ret;
  int value, iter = 0, len, th_value = 0;
  uint8_t data[256], thresholds[THRESHOLD_MAX] = {0};
  char *tmp_str = NULL, *data_str, path[256];

  VALIDATE(id);

  tid = ONLP_OID_ID_GET(id);

  path[0] = '\0';

  if ((tid == THERMAL_1_ON_PSU1) || (tid == THERMAL_1_ON_PSU2)) {
    sprintf(path,
            "%s"
            "psu%d_temp",
            PSU_SYSFS_PATH, tid - THERMAL_20_ON_MAIN_BROAD);
  } else {
    sprintf(path,
            "%s"
            "temp%d",
            THERMAL_SYSFS_PATH, tid);
  }

  /* Set the onlp_oid_hdr_t and capabilities */
  *info = linfo[tid];

  info->caps = ONLP_THERMAL_CAPS_GET_TEMPERATURE;
  info->thresholds.warning = 0;
  info->thresholds.error = 0;
  info->thresholds.shutdown = 0;

  ret = onlp_file_read_int(&value, "%s_thresh_caps", path);
  if (ret < 0) {
    AIM_LOG_ERROR("Unable to read from (%s_thresh_caps)\r\n", path);
    return ONLP_STATUS_E_INTERNAL;
  }

  data[0] = '\0';
  if (onlp_file_read(&data[0], 256, &len, "%s_thresh", path) < 0) {
    AIM_LOG_ERROR("Unable to read from (%s_thresh)\r\n", path);
    return ONLP_STATUS_E_INTERNAL;
  }

  data_str = (char*)data;
  while ((tmp_str = strtok_r(data_str, "\n", &data_str))) {
    th_value = ONLPLIB_ATOI(tmp_str);
    thresholds[iter] = th_value;
    iter++;
  }

  if (value & THRESHOLD_HI_NC) {
    info->caps |= ONLP_THERMAL_CAPS_GET_WARNING_THRESHOLD;
    info->thresholds.warning = thresholds[3] * 1000;
  }
  if (value & THRESHOLD_HI_CR) {
    info->caps |= ONLP_THERMAL_CAPS_GET_ERROR_THRESHOLD;
    info->thresholds.error = thresholds[4] * 1000;
  }
  if (value & THRESHOLD_HI_NR) {
    info->caps |= ONLP_THERMAL_CAPS_GET_SHUTDOWN_THRESHOLD;
    info->thresholds.shutdown = thresholds[5] * 1000;
  }

#if 0
    if(tid == THERMAL_CPU_CORE) {
        int rv = onlp_file_read_int_max(&info->mcelsius, cpu_coretemp_files);
        return rv;
    }
#endif

  value = 0;
  ret = onlp_file_read_int(&value, ipmi_devfiles__[tid]);
  if (ret < 0) {
    AIM_LOG_ERROR("Unable to read from %s\r\n", ipmi_devfiles__[tid]);
    return ONLP_STATUS_E_INTERNAL;
  }

  info->mcelsius = value * 1000;

  return ONLP_STATUS_OK;
}
