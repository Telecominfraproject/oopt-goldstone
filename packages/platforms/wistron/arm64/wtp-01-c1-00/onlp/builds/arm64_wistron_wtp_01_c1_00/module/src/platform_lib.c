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

#include <sys/time.h>
#include <unistd.h>
#include <onlp/onlp.h>
#include <onlp/module.h>
#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <AIM/aim.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

#define PSU_MODEL_NAME_LEN 	14
#define PSU_SERIAL_NUMBER_LEN	14
#define PSU_NODE_MAX_PATH_LEN   64

#define FPGA_PCI_PATH "/sys/bus/pci/devices/0001:01:00.0/resource0"

#define DSP_ADDR				0x6a
#define DSP_READ_MAX_RETRY  	50000
#define DSP_WRITE_MAX_RETRY  	10 
#define DSP_BUSY_VALUE			1
#define DSP_NOBUSY_VALUE		0
#define NTT_I2C_CMD_WAIT_US  	100000

int dsp_exec_read(int bus, struct dsp_ctrl_s* dsp_ctrl, int *ret_val)
{
	int i = 0, ret = 0;
	uint8_t value = 0, offset = 0;

	if (ret_val) {
		*ret_val = 0;
	}

	/* Set the address to be read into register 0x10 ~ 0x13
	 */
	for (i = 0; i <= 3; i++) {
		offset = 0x10 + i;
		value  = (dsp_ctrl->value1 >> (8 * (3-i))) & 0xff;

		ret = onlp_i2c_writeb(bus, DSP_ADDR, offset, value, ONLP_I2C_F_FORCE);
		if (ret < 0) {
			DEBUG_PRINT("%s(%d): Failed to write data into bus(%d), addr(0x%x), offset(0x%x), value(0x%x), error code(%d)\r\n",
				   __FUNCTION__, __LINE__, bus, DSP_ADDR, offset, value, ret);
			return ret;
		}
	}

    // Trigger the read operation : write 0x2 to Reg0x1 of dsp controller
    offset = 0x01;
	value  = 0x02;
    ret    = onlp_i2c_writeb(bus, DSP_ADDR, offset, value, ONLP_I2C_F_FORCE);
	if (ret < 0) {
		DEBUG_PRINT("%s(%d): Failed to write data into bus(%d), addr(0x%x), offset(0x%x), value(0x%x), error code(%d)\r\n",
			   __FUNCTION__, __LINE__, bus, DSP_ADDR, offset, value, ret);
		return ret;
	}


    // Check Reg0x02 is changed to 0x0(Busy:0x01, no-busy:0x00)
    offset = 0x02;
    i 	   = DSP_READ_MAX_RETRY;
    while(i--) {
		ret = onlp_i2c_readb(bus, DSP_ADDR, offset, ONLP_I2C_F_FORCE);
		if (ret < 0) {
			DEBUG_PRINT("%s(%d): Failed to read data from bus(%d), addr(0x%x), offset(0x%x), value(0x%x), error code(%d)\r\n",
				   __FUNCTION__, __LINE__, bus, DSP_ADDR, offset, value, ret);
			return ret;
		}

		if (ret == DSP_BUSY_VALUE) {
			usleep(NTT_I2C_CMD_WAIT_US);
			continue;
		}

		break;
    }

    if (ret == DSP_BUSY_VALUE) {
        DEBUG_PRINT("%s(%d): Device is busy!! Not change to Idle\r\n", __FUNCTION__, __LINE__);
        return ONLP_STATUS_E_INTERNAL;   
    }


	/* Read the output data from register 0x20 ~ 0x23
	 */
	for (i = 0; i <= 3; i++) {
		offset = 0x20 + i;

		ret = onlp_i2c_readb(bus, DSP_ADDR, offset, ONLP_I2C_F_FORCE);
		if (ret < 0) {
			DEBUG_PRINT("%s(%d): Failed to read data from bus(%d), addr(0x%x), offset(0x%x), value(0x%x), error code(%d)\r\n",
				   __FUNCTION__, __LINE__, bus, DSP_ADDR, offset, value, ret);
			return ret;
		}

		if (ret_val) {
			*ret_val <<= 8;
			*ret_val |= (ret & 0xff);
		}
	}

	return ret;
}

int cfp_eeprom_read(int port, uint8_t data[256])
{
	int i, bus, ret = 0, value = 0;
	struct dsp_ctrl_s eeprom_cmd = {"R", 0x40008000};
	int i2c_bus[] = {41, 43, 45, 47, 49, 51, 53, 55};

	/* init dsp card */
	bus = i2c_bus[port % 8];

	/* read eeprom */
	for (i = 0; i < 256; i++) {
		ret = dsp_exec_read(bus, &eeprom_cmd, &value);

		if (ret < 0) {
			AIM_LOG_ERROR("Unable to read the eeprom of port (%d), check if it has been initialized properly by dsp_ctrl ", port);
			return ret;
		}

		DEBUG_PRINT("%s(%d): command:(%s0x%x), Value(0x%x)\r\n", __FUNCTION__, __LINE__, eeprom_cmd.type, eeprom_cmd.value1, value);
		data[i] = (value & 0xff);
		eeprom_cmd.value1++;
	}

	return 0;
}

card_type_t
get_card_type_by_slot(int slot)
{
    int type = CARD_TYPE_UNKNOWN;

    if (get_piu_presence (slot)) {
        /* Read PIU type */
        char *string = NULL;
        int len = onlp_file_read_str(&string, "%s""piu%d/piu_type", PIU_SYSFS_PATH, slot);
        if (string && len) {
            if (strncmp (string, "ACO", 3) == 0) {
                type = CARD_TYPE_ACO;
            }
            else if (strncmp (string, "DCO", 3) == 0) {
                type = CARD_TYPE_DCO;
            }
            else if (strncmp (string, "QSFP", 4) == 0) {
                type = CARD_TYPE_Q28;
            }
            else {
                type = CARD_TYPE_UNKNOWN;
            }
        }
        else {
            type = CARD_TYPE_UNKNOWN;
        }
    }
    else {
        type = CARD_TYPE_NOT_PRESENT;
    }

    return type;
}

card_type_t
get_card_type_by_port(int port)
{
    int slotno = 0;
    /* Get card slot of this port */
    slotno = GET_SLOTNO_FROM_PORT(port);

    return get_card_type_by_slot(slotno);
}

port_type_t
get_port_type (int port)
{
    card_type_t card_type;
    port_type_t port_type;

    if (port >= QSFP_PORT_BEGIN && port <= QSFP_PORT_END) {
	return PORT_TYPE_Q28;
    }

    /* Get card type to get correct port mapping table */
    card_type = get_card_type_by_port(port);
    switch (card_type) {
	case CARD_TYPE_Q28: port_type = PORT_TYPE_PIU_Q28; break;
	case CARD_TYPE_ACO: port_type = PORT_TYPE_PIU_ACO_200G; break;
	case CARD_TYPE_DCO: port_type = PORT_TYPE_PIU_DCO_200G; break;
	default: port_type = PORT_TYPE_UNKNOWN; break;
    }

    return port_type;
}

int fpga_open(fpga_context_t* fpga, off_t target, int type_width, int items_count)
{
    int map_size = 4096UL;
    off_t target_base;
    if ( fpga == NULL ) {
        return -1;
    }
    fpga->fd = open(FPGA_PCI_PATH, O_RDWR | O_SYNC);
    if ( fpga->fd < 0 ) {
        return -1;
    }
    target_base = target & ~(sysconf(_SC_PAGE_SIZE)-1);

    if (target + items_count*type_width - target_base > map_size)
        map_size = target + items_count*type_width - target_base;

    fpga->map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fpga->fd, target_base);
    //printf("[FPGA READ] target_base[%lx], target[%lx], type_width[%x], items_count[%x]\n", target_base, target, type_width, items_count); 
    if ( fpga->map_base == (void*)-1 ) {
        close(fpga->fd);
        fpga->fd = 0;
        return -1;
    }
    fpga->map_size = map_size;
    fpga->target = target;
    fpga->target_base = target_base;
    fpga->type_width = type_width;
    fpga->items_count = items_count;
    return 0;
}

int fpga_close(fpga_context_t* fpga) {
    if (fpga == NULL ) {
        return 0;
    }
    munmap(fpga->map_base, fpga->map_size);
    close(fpga->fd);
    fpga->fd = 0;
    fpga->map_base = NULL;
    fpga->map_size = 0;
    return 0;
}

int fpga_read(int offset, int *value)
{
    fpga_context_t ctx;
    int rv;
    void *virt_addr;
    rv = fpga_open(&ctx, offset, 4, 1);
    if ( rv < 0 ) return rv;
    virt_addr = ctx.map_base + ctx.target - ctx.target_base;
    *value = *((uint32_t *) virt_addr);
    //printf("[FPGA READ] map_base[%p], value[%x]\n", ctx.map_base, *value);
    return fpga_close(&ctx);
}

int fpga_write(int offset, int value)
{
    fpga_context_t ctx;
    int rv;
    void *virt_addr;
    rv = fpga_open(&ctx, offset, 4, 1);
    if ( rv < 0 ) return rv;
    virt_addr = ctx.map_base + ctx.target - ctx.target_base;
    *((uint32_t *) virt_addr) = value;
    return fpga_close(&ctx);
}

int fpga_set_bit_low(uint16_t offset, uint16_t bit)
{
    fpga_context_t ctx;
    int rv;
    void *virt_addr;
    uint32_t value;
    rv = fpga_open(&ctx, offset, 4, 1);
    if ( rv < 0 ) return rv;
    virt_addr = ctx.map_base + ctx.target - ctx.target_base;
    value = *((uint32_t *) virt_addr);
    value = value & ~(1 << bit);
    *((uint32_t *) virt_addr) = value;
    return fpga_close(&ctx);
}

int fpga_set_bit_high(uint16_t offset, uint16_t bit)
{
    fpga_context_t ctx;
    int rv;
    void *virt_addr;
    uint32_t value;
    rv = fpga_open(&ctx, offset, 4, 1);
    if ( rv < 0 ) return rv;
    virt_addr = ctx.map_base + ctx.target - ctx.target_base;
    value = *((uint32_t *) virt_addr);
    value = value | (1 << bit);
    *((uint32_t *) virt_addr) = value;
    return fpga_close(&ctx);
}

int
fpga_get_offset_for_xvr_presence (int port, int *register_offset,
	     	                  int *presence_offset)
{
    if (port >= QSFP_PORT_BEGIN && port <= QSFP_PORT_BEGIN + 7) {
        *register_offset = QSFP_PRES_OFFSET1;
	*presence_offset = 1;
    }
    else if (port >= (QSFP_PORT_BEGIN + 8) && port <= QSFP_PORT_END) {
        *register_offset = QSFP_PRES_OFFSET2;
	*presence_offset = 1;
    }
    else if (port >= PIU_PORT_BEGIN && port <= PIU_PORT_END) {
        *register_offset = PIU_MOD_PRES_OFFSET;
	*presence_offset = PIU_PORT_BEGIN;
    }
    else {
        return ONLP_STATUS_E_INTERNAL;
    }
    
    return ONLP_STATUS_OK;
}

int
get_piu_presence (int slotno)
{
    int rv, pres_val, is_present;

    rv = onlp_file_read_int(&pres_val, "%s/piu%d/piu_simulate_plug_out", PIU_SYSFS_PATH, slotno);
    if (rv == ONLP_STATUS_OK && pres_val != 0 ) {
        return 0;
    }

    rv = fpga_read (PIU_PRES_OFFSET, &pres_val);
    if (rv != ONLP_STATUS_OK) {
        return ONLP_STATUS_E_INTERNAL;
    }

    is_present = (((pres_val & (1 << (slotno - 1))) != 0)? 0 : 1);

    return is_present;
}

int
get_module_status (int slotno)
{
    int rv, pres_val, bit_pos;
    if (slotno == 1 || slotno == 2) {
        rv = fpga_read(0x1100090, &pres_val);
        if ( rv != ONLP_STATUS_OK ) {
            return ONLP_STATUS_E_INTERNAL;
        }
        bit_pos = slotno == 1 ? 8 : 5;
    } else if (slotno == 3 || slotno == 4) {
        rv = fpga_read(0x5100090, &pres_val);
        if ( rv != ONLP_STATUS_OK ) {
            return ONLP_STATUS_E_INTERNAL;
        }
        bit_pos = slotno == 3 ? 5 : 9;
    } else {
        return ONLP_STATUS_E_INVALID;
    }

    if ( (pres_val & (1 << bit_pos)) == 0 ) {
        return ONLP_MODULE_STATUS_PIU_DCO_PRESENT | ONLP_MODULE_STATUS_PIU_CFP2_PRESENT;
    }
    return ONLP_MODULE_STATUS_UNPLUGGED;
}


int get_psu_presence(int idx)
{
    int rv, pres_val, is_present;

    rv = onlp_file_read_int(&pres_val, "%s/psu%d_status", PSU_SYSFS_PATH, idx);
    if ((rv == ONLP_STATUS_OK) && (pres_val != 0)) {
        return 0;
    }

    is_present = pres_val;

    return is_present;

/*  hardware present pin can't reflect real PSU status
    int v;
    if ( fpga_read(0x8000090, &v) != ONLP_STATUS_OK ) {
        return -1;
    }
    if ( idx == 1 ) {
        return (v & (1 << 23)) == 0;
    } else if ( idx == 2 ) {
        return (v & (1 << 15)) == 0;
    }
    return -1;
    */
}

