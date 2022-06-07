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
#include <onlplib/i2c.h>
#include <onlplib/file.h>
#include <onlplib/sfp.h>
#include "platform_lib.h"

#define NUM_OF_PORT    16
#define QSFP28_PORT_FORMAT "/sys/bus/i2c/devices/%d-0050/eeprom"

/************************************************************
 *
 * SFPI Entry Points
 *
 ***********************************************************/
int
onlp_sfpi_init(void)
{
    /* Called at initialization time */
    return ONLP_STATUS_OK;
}

int
onlp_sfpi_bitmap_get(onlp_sfp_bitmap_t* bmap)
{
    int p;
    AIM_BITMAP_CLR_ALL(bmap);

    for(p = 1; p <= NUM_OF_PORT; p++) {
        AIM_BITMAP_SET(bmap, p);
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_is_present(int port)
{
    if ( port < 1 || port > NUM_OF_PORT ) {
        return ONLP_STATUS_E_INVALID;
    }
    int value, rv;
    rv = fpga_read(0x09000110, &value);
    if ( rv < 0 ) {
        return ONLP_STATUS_E_INTERNAL;
    }
    return !(value & (1 << (port-1)));
}

int
onlp_sfpi_presence_bitmap_get(onlp_sfp_bitmap_t* dst)
{
    int value, rv, i;
    rv = fpga_read(0x09000110, &value);
    if ( rv < 0 ) {
        return ONLP_STATUS_E_INTERNAL;
    }

    for(i = 1; i <= NUM_OF_PORT; i++) {
        AIM_BITMAP_MOD(dst, i, !(value & (1 << (i-1))));
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_eeprom_read(int port, uint8_t data[256])
{
    if (!onlp_sfpi_is_present(port)) {
        return ONLP_STATUS_E_INTERNAL;
    }
    char eeprom_path[512];
    snprintf(eeprom_path, sizeof(eeprom_path), QSFP28_PORT_FORMAT, port+1);

    if (onlplib_sfp_eeprom_read_file(eeprom_path, data) != 0) {
        AIM_LOG_INFO("Unable to read eeprom from port(%d)\r\n", port);
        return ONLP_STATUS_E_INTERNAL;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sfpi_denit(void)
{
    return ONLP_STATUS_OK;
}
