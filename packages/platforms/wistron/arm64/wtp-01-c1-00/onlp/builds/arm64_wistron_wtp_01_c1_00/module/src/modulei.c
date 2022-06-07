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
#include <onlp/platformi/modulei.h>
#include <onlplib/file.h>
#include "platform_lib.h"

#define MODULE1_ID         1
#define MODULE2_ID         2
#define MODULE3_ID         3
#define MODULE4_ID         4

#define CFP2DCO_SYSFS_PATH "/sys/class/cfp2dco/"

static onlp_module_info_t minfo[] = {
    {}, /* not used */
    {
      {ONLP_MODULE_ID_CREATE(MODULE1_ID), "CFP2DCO-1", 0},
      0,
    },
    {
      {ONLP_MODULE_ID_CREATE(MODULE2_ID), "CFP2DCO-2", 0},
      0,
    },
    {
      {ONLP_MODULE_ID_CREATE(MODULE3_ID), "CFP2DCO-3", 0},
      0,
    },
    {
      {ONLP_MODULE_ID_CREATE(MODULE4_ID), "CFP2DCO-4", 0},
      0,
    }
};

int onlp_modulei_info_get(onlp_oid_t id, onlp_module_info_t *info) {
    uint32_t status = 0;
    *info = minfo[ONLP_OID_ID_GET(id)];
    onlp_modulei_status_get(id, &status);
    info->status = status;
    return ONLP_STATUS_OK;
}

int onlp_modulei_status_get(onlp_oid_t id, uint32_t* status) {
    /* Here ID can be used to identify the slotno */
    char *s = NULL;
    int slot = ONLP_OID_ID_GET(id);
    int rv = onlp_file_read_str(&s, "%scfp2dco%d/part_number", CFP2DCO_SYSFS_PATH, slot);
    aim_free(s);
    *status = rv > 0 ? (ONLP_MODULE_STATUS_PIU_DCO_PRESENT | ONLP_MODULE_STATUS_PIU_CFP2_PRESENT) : ONLP_MODULE_STATUS_UNPLUGGED;
    return ONLP_STATUS_OK;
}
