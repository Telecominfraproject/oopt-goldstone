/************************************************************
 * <bsn.cl fy=2014 v=onl>
 *
 *        Copyright 2014, 2015 Big Switch Networks, Inc.
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
#include "onlp_mdio.h"

//#if ONLPLIB_CONFIG_INCLUDE_MDIO == 1

#include <onlplib/file.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <onlp/onlp.h>
#include "arm64_wistron_wtp_01_c1_00_log.h"

/*
int
onlp_mdio_open(int psu_id, uint8_t cob)
{
    return ONLP_STATUS_E_MDIO;
}
*/

/*
static int parse_phyaddr_devtype(char *text, config_mdio *loc_config)
{
    unsigned long port, dev;
    char *end;

    //port = strtoul(text, &end, 0);
    if (!end[0]) {
        // simple phy address 
        loc_config->phy_addr = port;
        loc_config->is_C45 = 0; //is C22
        return 0;
    }

    if (end[0] != ':') {
        // not clause 45 either
        return 1;
    }

    dev = strtoul(end + 1, &end, 0);
    if (end[0])
        return 1;

    loc_config->phy_addr = port;
    loc_config->dev_addr = dev;
    loc_config->is_C45 = 1; //is C45

    return 0;
}
*/

int
onlp_mdio_read_c22(uint32_t addr, uint32_t reg, uint16_t *value)
{
    int fd;
    config_mdio loc_config;

    fd = open("/dev/ENETC_MDIO_DEV", O_RDWR);

    if( fd < 0 ) {
        AIM_LOG_ERROR("Character Device[/dev/ENETC_MDIO_DEV] not opened [%d] ::: Device Driver Not Inserted\n",fd);
        return -1;
    } 
/*
    ret = parse_phyaddr_devtype(addr, &loc_config);
    if (ret) {
	goto error;
    }
  */  
    loc_config.phy_addr = addr;
    loc_config.dev_addr = addr;
    loc_config.reg_addr = reg;
    loc_config.is_C45 = 0; //is C22
    //loc_config.reg_addr = strtoul(reg, NULL, 0);


    if (ioctl(fd, ENETC_MDIO_READ,&loc_config) == -1)
    {
        //clean_stdin();
        AIM_LOG_ERROR("MDIO address[%x] read failed\n", loc_config.phy_addr);
	goto error;
    }

    *value = loc_config.value;

    close (fd);

    return loc_config.value;

 error:
    close(fd);

    return ONLP_STATUS_E_INVALID;
}


int
onlp_mdio_write_c22(uint32_t addr, uint32_t reg, uint16_t value)
{
    int fd;
    config_mdio loc_config;

    fd = open("/dev/ENETC_MDIO_DEV", O_RDWR);

    if( fd < 0 ) {
        AIM_LOG_ERROR("Character Device[/dev/ENETC_MDIO_DEV] not opened [%d] ::: Device Driver Not Inserted\n",fd);
        return -1;
    }
/*
    ret = parse_phyaddr_devtype(addr, &loc_config);
    if (ret) {
	goto error;
    }
*/

    loc_config.phy_addr = addr;
    loc_config.dev_addr = addr;
    loc_config.reg_addr = reg;
    loc_config.is_C45 = 0; //is C22
    //loc_config.reg_addr = strtoul(reg, NULL, 0);
    loc_config.value = value;

    if (ioctl(fd, ENETC_MDIO_WRITE,&loc_config) == -1)
    {
        //clean_stdin();
        AIM_LOG_ERROR("MDIO address[%x] write failed\n", loc_config.phy_addr);
	goto error;
    }


    close (fd);

    return loc_config.value;

 error:
    close(fd);

    return ONLP_STATUS_E_INVALID;
}


int
onlp_mdio_read_c45(uint32_t addr, uint32_t reg, uint16_t *value)
{
    int fd;
    config_mdio loc_config;

    fd = open("/dev/ENETC_MDIO_DEV", O_RDWR);

    if( fd < 0 ) {
        AIM_LOG_ERROR("Character Device[/dev/ENETC_MDIO_DEV] not opened [%d] ::: Device Driver Not Inserted\n",fd);
        //printf("Character Device[/dev/ENETC_MDIO_DEV] not opened [%d] ::: Device Driver Not Inserted\n",fd);
        return -1;
    } 
/*
    ret = parse_phyaddr_devtype(addr, &loc_config);
    if (ret) {
	goto error;
    }
  */  
    loc_config.phy_addr = addr;
    loc_config.dev_addr = 1;
    loc_config.reg_addr = reg;
    loc_config.is_C45 = 1;
    //loc_config.reg_addr = strtoul(reg, NULL, 0);


    if (ioctl(fd, ENETC_MDIO_READ,&loc_config) == -1)
    {
        //clean_stdin();
        AIM_LOG_ERROR("MDIO address[%x] read failed\n", loc_config.phy_addr);
        //printf("MDIO address[%x] read failed\n", loc_config.phy_addr);
	goto error;
    }

    //printf("[MDIO_READ] addr[%x]: reg[%x], value[%x]\n", addr, reg, loc_config.value);
    *value = loc_config.value;

    close (fd);

    return loc_config.value;

 error:
    close(fd);

    return ONLP_STATUS_E_INVALID;
}


int
onlp_mdio_write_c45(uint32_t addr, uint32_t reg, uint16_t value)
{
    int fd;
    config_mdio loc_config;

    fd = open("/dev/ENETC_MDIO_DEV", O_RDWR);

    if( fd < 0 ) {
        AIM_LOG_ERROR("Character Device[/dev/ENETC_MDIO_DEV] not opened [%d] ::: Device Driver Not Inserted\n",fd);
        return -1;
    }
/*
    ret = parse_phyaddr_devtype(addr, &loc_config);
    if (ret) {
	goto error;
    }
*/

    loc_config.phy_addr = addr;
    loc_config.dev_addr = 1;
    loc_config.reg_addr = reg;
    loc_config.is_C45 = 1;
    //loc_config.reg_addr = strtoul(reg, NULL, 0);
    loc_config.value = value;

    if (ioctl(fd, ENETC_MDIO_WRITE,&loc_config) == -1)
    {
        //clean_stdin();
        AIM_LOG_ERROR("MDIO address[%x] write failed\n", loc_config.phy_addr);
	goto error;
    }


    close (fd);

    return loc_config.value;

 error:
    close(fd);

    return ONLP_STATUS_E_INVALID;
}

//#endif /* ONLPLIB_CONFIG_INCLUDE_MDIO */
