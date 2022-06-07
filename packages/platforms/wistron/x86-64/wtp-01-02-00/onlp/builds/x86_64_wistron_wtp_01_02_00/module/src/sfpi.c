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
#include <onlp/platformi/sfpi.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <onlplib/sfp.h>

#include "platform_lib.h"

#define NUM_OF_PORT 20
static const int qsfp_mux_index[] = {12, 11, 14, 13, 16, 15,
                                     18, 17, 20, 19, 22, 21};

#define QSFP_BUS_INDEX(port, port_type) \
  (port_type == PORT_TYPE_Q28           \
       ? (qsfp_mux_index[port - 1])     \
       : GET_SLOTNO_FROM_PORT(port))

#define QSFP28_PORT_FORMAT "/sys/bus/i2c/devices/%d-0050/eeprom"
#define PIU_QSFP28_PORT_FORMAT "/sys/class/piu/piu%d/qsfp28_%d_eeprom"

/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/
int onlp_sfpi_init(void) {
  /* Called at initialization time */
  return ONLP_STATUS_OK;
}

int onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap) {
  int p;
  AIM_BITMAP_CLR_ALL(bmap);

  for (p = 1; p <= NUM_OF_PORT; p++) {
    AIM_BITMAP_SET(bmap, p);
  }

  return ONLP_STATUS_OK;
}

int onlp_sfpi_is_present(int port) {
  /*
   * Return 1 if present.
   * Return 0 if not present.
   * Return < 0 if error.
   */
  return get_sff_presence(port);
}

int onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst) {
  int p = 1;
  int rc = 0;

  for (p = 1; p <= NUM_OF_PORT; p++) {
    rc = onlp_sfpi_is_present(p);
    AIM_BITMAP_MOD(dst, p, (1 == rc) ? 1 : 0);
  }

  return ONLP_STATUS_OK;
}

static inline unsigned char str2hexnum(unsigned char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;

  return 0; /* foo */
}

int onlp_sfpi_eeprom_read(int port, uint8_t data[256]) {
  int ret = ONLP_STATUS_OK;
  port_type_t port_type = get_port_type(port);

  char eeprom_path[512];
  memset(eeprom_path, 0, sizeof(eeprom_path));

  if (!onlp_sfpi_is_present(port)) {
    return ret;
  }

  switch (port_type) {
    case PORT_TYPE_Q28:
      snprintf(eeprom_path, sizeof(eeprom_path), QSFP28_PORT_FORMAT,
               QSFP_BUS_INDEX(port, port_type));
      break;
    case PORT_TYPE_PIU_Q28:
      snprintf(eeprom_path, sizeof(eeprom_path), PIU_QSFP28_PORT_FORMAT,
               QSFP_BUS_INDEX(port, port_type), (port % 2) ? 1 : 2);
      break;
    default:
      return ret;
  }

  if (onlplib_sfp_eeprom_read_file(eeprom_path, data) != 0) {
    AIM_LOG_INFO("Unable to read eeprom from port(%d)\r\n", port);
    return ONLP_STATUS_E_INTERNAL;
  }

  return ret;
}

int onlp_sfpi_dev_readb(int port, uint8_t devaddr, uint8_t addr) {
  port_type_t port_type = get_port_type(port);
  int bus = QSFP_BUS_INDEX(port, port_type);
  return onlp_i2c_readb(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int onlp_sfpi_dev_writeb(int port, uint8_t devaddr, uint8_t addr,
                         uint8_t value) {
  port_type_t port_type = get_port_type(port);
  int bus = QSFP_BUS_INDEX(port, port_type);
  return onlp_i2c_writeb(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int onlp_sfpi_dev_readw(int port, uint8_t devaddr, uint8_t addr) {
  port_type_t port_type = get_port_type(port);
  int bus = QSFP_BUS_INDEX(port, port_type);
  return onlp_i2c_readw(bus, devaddr, addr, ONLP_I2C_F_FORCE);
}

int onlp_sfpi_dev_writew(int port, uint8_t devaddr, uint8_t addr,
                         uint16_t value) {
  port_type_t port_type = get_port_type(port);
  int bus = QSFP_BUS_INDEX(port, port_type);
  return onlp_i2c_writew(bus, devaddr, addr, value, ONLP_I2C_F_FORCE);
}

int onlp_sfpi_denit(void) { return ONLP_STATUS_OK; }
