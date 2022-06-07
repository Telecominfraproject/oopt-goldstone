/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *           Copyright 2014 Big Switch Networks, Inc.
 *           Copyright 2017 Accton Technology Corporation.
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
#include <fcntl.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/psui.h>
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/thermali.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <unistd.h>

#include "nvme-ioctl.h"
#include "platform_lib.h"
#include "x86_64_wistron_wtp_01_02_00_int.h"
#include "x86_64_wistron_wtp_01_02_00_log.h"

#define NVME_DEV "/dev/nvme0n1"

const char* onlp_sysi_platform_get(void) {
  return "x86-64-wistron-wtp-01-02-00-r0";
}

int onlp_sysi_onie_data_get(uint8_t** data, int* size) {
  uint8_t* rdata = aim_zmalloc(256);

  if (onlp_file_read(rdata, 256, size, IDPROM_PATH) == ONLP_STATUS_OK) {
    if (*size == 256) {
      *data = rdata;
      return ONLP_STATUS_OK;
    }
  }

  aim_free(rdata);
  *size = 0;
  return ONLP_STATUS_E_INTERNAL;
}

int onlp_sysi_oids_get(onlp_oid_t* table, int max) {
  int i;
  onlp_oid_t* e = table;
  memset(table, 0, max * sizeof(onlp_oid_t));

  /* Thermal sensors on the chassis */
  for (i = 1; i <= CHASSIS_THERMAL_COUNT; i++) {
    *e++ = ONLP_THERMAL_ID_CREATE(i);
  }

  /* LEDs on the chassis */
  for (i = 1; i <= CHASSIS_LED_COUNT; i++) {
    *e++ = ONLP_LED_ID_CREATE(i);
  }

  /* PSUs on the chassis */
  for (i = 1; i <= CHASSIS_PSU_COUNT; i++) {
    *e++ = ONLP_PSU_ID_CREATE(i);
  }

  /* Fans on the chassis */
  for (i = 1; i <= CHASSIS_FAN_COUNT; i++) {
    *e++ = ONLP_FAN_ID_CREATE(i);
  }

  /* Modules[PIUs] on the chassis */
  for (i = 1; i <= CHASSIS_MODULE_COUNT; i++) {
    *e++ = ONLP_MODULE_ID_CREATE(i);
  }

  return 0;
}

int onlp_sysi_platform_info_get(onlp_platform_info_t* pi) {
  return ONLP_STATUS_OK;
}

void onlp_sysi_platform_info_free(onlp_platform_info_t* pi) {
  aim_free(pi->cpld_versions);
}

static int join(char* buf, int bufsize, int* values, int size, char delim) {
  int i, cnt;
  char* head = buf;
  for (i = 0; i < size; i++) {
    cnt = snprintf(head, bufsize - (head - buf), "%d", values[i]);
    if (cnt == EOF) {
      return -1;
    }
    head += cnt;
    if (i != (size - 1)) {
      *head++ = delim;
    }
  }
  return 0;
}

static int get_nvme_temp(const char* dev) {
  int fd, ret;
  struct nvme_smart_log log;
  fd = open(dev, O_RDONLY);
  ret = nvme_smart_log(fd, NVME_NSID_ALL, &log);
  close(fd);
  if (ret) {
    return 0;
  }
  return log.temperature[1] << 8 | log.temperature[0];
}

int onlp_sysi_platform_manage_fans(void) {
  int i, value = 0, ret;
  int values[28] = {};
  char buf[256] = {};

  for (i = 1; i < 5; i++) {
    ret = onlp_file_read_int(
        &value, "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp%d_input",
        i);
    if (ret == 0) {
      if (i == 1) {
        values[0] = value / 1000;
      } else {
        values[2 + i] = value / 1000;
      }
    }
  }

  // DIMM temperature
  ret = onlp_file_read_int(
      &value, "/sys/bus/i2c/devices/1-0018/hwmon/hwmon1/temp1_input");
  if (ret == 0) {
    values[1] = value / 1000;
  }

  // BCM switch temperature
  ret = onlp_file_read_int(&value, "/run/bcm/temp_max_peak");
  if (ret == 0) {
    values[2] = value / 1000;
  }

  // M.2 temperature
  values[3] = get_nvme_temp(NVME_DEV);

  for (i = 1; i < 5; i++) {
    ret = onlp_file_read_int(&value, "/sys/class/piu/piu%d/piu_temp", i);
    if (ret == 0) {
      values[7 + i] = value;
    }
    ret = onlp_file_read_int(&value, "/sys/class/piu/piu%d/cfp2_cage_temp", i);
    if (ret == 0) {
      values[11 + i] = value;
    }
  }

  ret = join(buf, sizeof(buf), values, sizeof(values) / sizeof(int), ' ');
  if (ret) {
    return -1;
  }

  return onlp_file_write_str(
      buf,
      "/sys/devices/platform/wtp_01_02_00_thermal/bmc_internal_sensor_reading");
}
