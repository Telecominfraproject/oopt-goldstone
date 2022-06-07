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
#include "platform_lib.h"

#include <AIM/aim.h>
#include <onlp/onlp.h>
#include <onlp/module.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

card_type_t get_card_type_by_slot(int slot) {
  int type = CARD_TYPE_UNKNOWN;

  if (get_piu_presence(slot)) {
    /* Read PIU type */
    char *string = NULL;
    int len = onlp_file_read_str(&string,
                                 "%s"
                                 "piu%d/piu_type",
                                 PIU_SYSFS_PATH, slot);
    if (string && len) {
      if (strncmp(string, "ACO", 3) == 0) {
        type = CARD_TYPE_ACO;
      } else if (strncmp(string, "DCO", 3) == 0) {
        type = CARD_TYPE_DCO;
      } else if (strncmp(string, "QSFP", 4) == 0) {
        type = CARD_TYPE_Q28;
      } else {
        type = CARD_TYPE_UNKNOWN;
      }
    } else {
      type = CARD_TYPE_UNKNOWN;
    }
  } else {
    type = CARD_TYPE_NOT_PRESENT;
  }

  return type;
}

card_type_t get_card_type_by_port(int port) {
  int slotno = 0;
  /* Get card slot of this port */
  slotno = GET_SLOTNO_FROM_PORT(port);

  return get_card_type_by_slot(slotno);
}

port_type_t get_port_type(int port) {
  card_type_t card_type;
  port_type_t port_type;

  if (port >= QSFP_PORT_BEGIN && port <= QSFP_PORT_END) {
    return PORT_TYPE_Q28;
  }

  /* Get card type to get correct port mapping table */
  card_type = get_card_type_by_port(port);
  switch (card_type) {
    case CARD_TYPE_Q28:
      port_type = PORT_TYPE_PIU_Q28;
      break;
    case CARD_TYPE_ACO:
      port_type = PORT_TYPE_PIU_ACO_200G;
      break;
    case CARD_TYPE_DCO:
      port_type = PORT_TYPE_PIU_DCO_200G;
      break;
    default:
      port_type = PORT_TYPE_UNKNOWN;
      break;
  }

  return port_type;
}

static int fpga_read(int offset, int *value) {
  int fd = open("/sys/bus/i2c/devices/0-0030/fpga", O_RDONLY);
  if (fd < 0) {
      return ONLP_STATUS_E_MISSING;
  }
  if (offset > 31 || offset < 0) {
      return ONLP_STATUS_E_PARAM;
  }
  uint8_t data[32];

  int nrd = read(fd, data, 32);
  close(fd);

  if (nrd != 32) {
      AIM_LOG_INTERNAL("Failed to read FPGA register");
      return ONLP_STATUS_E_INTERNAL;
  }

  *value = data[offset];
  return ONLP_STATUS_OK;
}

static int fpga_get_offset_for_xvr_presence(int port, int *register_offset,
                                            int *presence_offset) {
  if (port >= QSFP_PORT_BEGIN && port <= QSFP_PORT_BEGIN + 7) {
    *register_offset = QSFP_PRES_OFFSET1;
    *presence_offset = 1;
  } else if (port >= (QSFP_PORT_BEGIN + 8) && port <= QSFP_PORT_END) {
    *register_offset = QSFP_PRES_OFFSET2;
    *presence_offset = 1;
  } else if (port >= PIU_PORT_BEGIN && port <= PIU_PORT_END) {
    *register_offset = PIU_MOD_PRES_OFFSET;
    *presence_offset = PIU_PORT_BEGIN;
  } else {
    return ONLP_STATUS_E_INTERNAL;
  }

  return ONLP_STATUS_OK;
}

int get_piu_presence(int slotno) {
  int rv, pres_val, is_present;

  rv = onlp_file_read_int(&pres_val, "%s/piu%d/piu_simulate_plug_out",
                          PIU_SYSFS_PATH, slotno);
  if (rv == ONLP_STATUS_OK && pres_val != 0) {
    return 0;
  }

  rv = fpga_read(PIU_PRES_OFFSET, &pres_val);
  if (rv != ONLP_STATUS_OK) {
    return 0;
  }

  is_present = (((pres_val & (1 << (slotno - 1))) != 0) ? 0 : 1);

  return is_present;
}

int get_cfp2_presence(int slotno) {
  int pres_val = 0;

  onlp_file_read_int(&pres_val, "%s/piu%d/cfp2_exists", PIU_SYSFS_PATH, slotno);
  return pres_val;
}

int get_sff_presence(int port) {
  int rv, pres_val, pres_index, presence_offset, register_offset;
  port_type_t port_type = get_port_type(port);

  /* This presence is only for qsfp */
  if ((PORT_TYPE_Q28 != port_type) && (PORT_TYPE_PIU_Q28 != port_type)) {
    return 0;
  }

  rv = fpga_get_offset_for_xvr_presence(port, &register_offset,
                                        &presence_offset);
  if (rv != ONLP_STATUS_OK) {
    return ONLP_STATUS_E_INTERNAL;
  }

  rv = fpga_read(register_offset, &pres_val);
  if (rv != ONLP_STATUS_OK) {
    return ONLP_STATUS_E_INTERNAL;
  }

  pres_index = (port - presence_offset) % 8;
  rv = (((pres_val & (1 << pres_index)) != 0) ? 0 : 1);
  return rv;
}

int get_module_status(int slotno) {
  int status = 0;
  int type = get_card_type_by_slot(slotno);

  switch (type) {
    case CARD_TYPE_NOT_PRESENT:
      status = ONLP_MODULE_STATUS_UNPLUGGED;
      break;
    case CARD_TYPE_ACO:
      status = ONLP_MODULE_STATUS_PIU_ACO_PRESENT;
      /* intentional fallthrough */
    case CARD_TYPE_DCO:
      if (type == CARD_TYPE_DCO)
          status = ONLP_MODULE_STATUS_PIU_DCO_PRESENT;

      if (get_cfp2_presence(slotno)) {
        status |= ONLP_MODULE_STATUS_PIU_CFP2_PRESENT;
      }
      break;
    case CARD_TYPE_Q28:
      status = ONLP_MODULE_STATUS_PIU_QSFP28_PRESENT;
      if (get_sff_presence(PIU_PORT_BEGIN + (slotno-1)*2) == 1) {
        status |= ONLP_MODULE_STATUS_PIU_QSFP28_1_PRESENT;
      }
      if (get_sff_presence(PIU_PORT_BEGIN + ((slotno-1)*2) + 1) == 1) {
        status |= ONLP_MODULE_STATUS_PIU_QSFP28_2_PRESENT;
      }
      break;
    default:
      status = ONLP_MODULE_STATUS_UNPLUGGED;
  }

  return status;
}

int get_psu_presence(int idx)
{
    int v;
    if ( fpga_read(0x8, &v) != ONLP_STATUS_OK ) {
        return -1;
    }
    if ( idx == 1 ) {
        return (v & (1 << 3)) == 0;
    } else if ( idx == 2 ) {
        return (v & (1 << 6)) == 0;
    }
    return -1;
}
