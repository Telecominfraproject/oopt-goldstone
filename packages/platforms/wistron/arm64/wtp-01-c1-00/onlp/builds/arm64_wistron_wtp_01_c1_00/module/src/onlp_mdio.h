/************************************************************
 * <bsn.cl v=2014 v=onl>
 *
 *           Copyright 2015 Big Switch Networks, Inc.
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
 * Common MDIO processing for all platform implementations.
 *
 ***********************************************************/
#ifndef __ONLP_MDIO_H__
#define __ONLP_MDIO_H__

#include <onlplib/onlplib_config.h>
#include <math.h>
#include <float.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>



//#if ONLPLIB_CONFIG_INCLUDE_MDIO == 1

#define MPCI_IOC_MAGIC 'm'

typedef struct {

	bool is_C45;
	bool is_disable_mdio;
	uint32_t phy_addr;
	uint32_t dev_addr;
	uint32_t reg_addr;
	uint16_t value;

}config_mdio;

#define ENETC_MDIO_WRITE               _IOWR(MPCI_IOC_MAGIC, 0, config_mdio)
#define ENETC_MDIO_READ                _IOWR(MPCI_IOC_MAGIC, 1, config_mdio)
#define ENETC_MDIO_CONFIG	       _IOWR(MPCI_IOC_MAGIC, 2, config_mdio)

#define SFP1_PHYADDRESS		9
#define SFP2_PHYADDRESS 	8
#define SFP_LED_REGISTER	0x1D
#define SFP_LED_BIT_GREEN       0
#define SFP_LED_BIT_YELLOW      12

#define CFP2_1_PHYADDRESS       0
#define CFP2_2_PHYADDRESS       1
#define CFP2_3_PHYADDRESS       2
#define CFP2_4_PHYADDRESS       3

/**
 * @brief Read i2c data.
 * @param bus The i2c bus number.
 * @param addr The slave address.
 * @param offset The byte offset.
 * @param size The byte count.
 * @param rdata [out] Receives the data.
 * @param flags See ONLP_I2C_F_*
 * @note This function reads a byte at a time.
 * See onlp_i2c_read_block() for block reads.
 */


int onlp_mdio_read_c22(uint32_t addr, uint32_t reg_addr, uint16_t* value);
int onlp_mdio_write_c22(uint32_t addr, uint32_t reg_addr, uint16_t value);

int onlp_mdio_read_c45(uint32_t addr, uint32_t reg_addr, uint16_t* value);
int onlp_mdio_write_c45(uint32_t addr, uint32_t reg_addr, uint16_t value);


//#endif /* ONLPLIB_CONFIG_INCLUDE_MDIO */

#endif /* __ONLP_MDIO_H__ */
