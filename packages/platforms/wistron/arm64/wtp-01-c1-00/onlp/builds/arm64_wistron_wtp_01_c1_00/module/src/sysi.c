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
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

#include <onlplib/file.h>
#include <onlplib/i2c.h>
#include <onlp/platformi/sysi.h>
#include <onlp/platformi/ledi.h>
#include <onlp/platformi/thermali.h>
#include <onlp/platformi/fani.h>
#include <onlp/platformi/psui.h>
#include <onlp/platformi/sfpi.h>
#include <onlp/module.h>
#include "platform_lib.h"

#include "arm64_wistron_wtp_01_c1_00_int.h"
#include "arm64_wistron_wtp_01_c1_00_log.h"
#include "onlp_mdio.h"

#include "nvme-ioctl.h"

#define NVME_DEV "/dev/nvme0n1"

#define SPEED_100_PERCENTAGE	100
#define FAN_SPEED_MAX_PERCENTAGE    100
#define FAN_SPEED_MIN_PERCENTAGE    30
#define FAN_MIN_RPM		    5000
#define QSFP28_WARNING_THRESHOLD    90000
#define QSFP28_ERROR_THRESHOLD      100000
#define QSFP28_SHUTDOWN_THRESHOLD   105000
#define CFP2_WARNING_THRESHOLD	    90000
#define CFP2_ERROR_THRESHOLD        100000
#define CFP2_SHUTDOWN_THRESHOLD     105000

#define FAN_SLOWDOWN_COUNTER	5


//#define THERMAL_VERSION		1


//static uint16_t Ka = 6, Kb = 100;
// actual value is Ka/100, Kb/10
// 9 thermal sensors for M.2 SSD
// 10 thermal sensors for CFP2
// 11 thermal sensors for QSFP28
// FAN TABLE V0.1
static int Ka[CHASSIS_THERMAL_COUNT+3] = {300,300,140,500,666,680,750,680,750,285,500,660};
static int Kb[CHASSIS_THERMAL_COUNT+3] = {-1520,-1520,30,-2579,-3600,-4220,-4707,-4220,-4707,-914,-2550,-3600};

// default fan PWM 30%
static int old_pct = 30;
static int fan_counter = 0;
static int fan_index = -1;
static int fan_offset = 0;

const char*
onlp_sysi_platform_get(void)
{
    // TODO get this info from system EEPROM
    return "arm64-wistron-wtp-01-c1-00-r0";
}

int
onlp_sysi_onie_data_get(uint8_t** data, int* size)
{
    uint8_t* rdata = aim_zmalloc(256);

    if(onlp_file_read(rdata, 256, size, IDPROM_PATH) == ONLP_STATUS_OK) {
        if(*size == 256) {
            *data = rdata;
            return ONLP_STATUS_OK;
        }
    }

    aim_free(rdata);
    *size = 0;
    return ONLP_STATUS_E_INTERNAL;
}

int
onlp_sysi_oids_get(onlp_oid_t* table, int max)
{
    int i;
    onlp_oid_t* e = table;
    memset(table, 0, max*sizeof(onlp_oid_t));

    /* Thermal sensors on the chassis */
    for (i = 1; i <= CHASSIS_THERMAL_COUNT; i++) {
        *e++ = ONLP_THERMAL_ID_CREATE(i);
    }

    /* PSUs on the chassis */
    for (i = 1; i <= CHASSIS_PSU_COUNT; i++) {
        *e++ = ONLP_PSU_ID_CREATE(i);
    }

    /* LEDs on the chassis */
    for (i = 1; i <= CHASSIS_LED_COUNT+CFP2_LED_COUNT+QSFP28_LED_COUNT; i++) {
      *e++ = ONLP_LED_ID_CREATE(i);
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

int
onlp_sysi_platform_info_get(onlp_platform_info_t* pi)
{
    return ONLP_STATUS_OK;
}

void
onlp_sysi_platform_info_free(onlp_platform_info_t* pi)
{
    aim_free(pi->cpld_versions);
}

static int get_nvme_temp(const char *dev)
{
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

int
onlp_sysi_platform_set_fans(int index, int ka, int kb, int offset)
{
    printf("%s: index[%d], ka[%d.%d], kb[%d.%d], offset[%3d.%3d]\n", __FUNCTION__, index, ka/100, ka%100, kb/10, kb%10, offset/1000, offset%1000);
    if (index > CHASSIS_THERMAL_COUNT)
	return ONLP_STATUS_E_INVALID;

    // Ka, kb value too high
    if ((ka/2+kb/10) > 150)
	return ONLP_STATUS_E_INVALID;

    Ka[index] = ka;
    Kb[index] = kb;

    if ( offset != 0 )
    {
	fan_index = index;
	fan_offset = offset;
    }
    else
    {
	fan_index = -1;
	fan_offset = 0;
    }

    return ONLP_STATUS_OK;
}

int
onlp_sysi_platform_manage_fans(void)
{
    int i = 0, port;
    int new_pct, temp_pct, thermal_temp;
    int highest_temp = 0, highest_pct = 0;
    int qsfp_temp = 0;
    int rv = 0;
    onlp_thermal_info_t thermal[CHASSIS_THERMAL_COUNT+2];
#ifdef THERMAL_VERSION
    time_t timep;
#endif
    uint8_t data[256];
    uint16_t data16;

    int pid, fid;
    onlp_fan_info_t fan_info;
    onlp_psu_info_t psu_info[NUM_OF_PSU_ON_MAIN_BROAD];

    // PSU / FAN failed control
    // PSU Status
    for(pid = 0; pid < NUM_OF_PSU_ON_MAIN_BROAD; pid++)
    {
        rv = onlp_psui_info_get(ONLP_PSU_ID_CREATE(pid+1), &psu_info[pid]);

        if ( (rv == ONLP_STATUS_OK) && !(psu_info[pid].status & ONLP_PSU_STATUS_PRESENT) )
        {
            new_pct = FAN_SPEED_MAX_PERCENTAGE;

            AIM_LOG_ERROR("PSU %d failed!! status[%x]", pid+1, psu_info[pid].status);
            goto set_temp;
        }
    }

    // FAN status
    for(pid = 0; pid < NUM_OF_PSU_ON_MAIN_BROAD; pid++)
    {
        for (fid = 0; fid < NUM_OF_FAN_ON_PSU_BROAD; fid++)
        {
            if (psu_info[pid].status & ONLP_PSU_STATUS_PRESENT)
            {
                rv = onlp_fani_info_get(ONLP_FAN_ID_CREATE(pid*NUM_OF_FAN_ON_PSU_BROAD+fid+1), &fan_info);

                if (rv == ONLP_STATUS_OK)
                {
                    if ( !(fan_info.status & ONLP_FAN_STATUS_PRESENT) || (fan_info.rpm <= FAN_MIN_RPM) )
                    {
                        new_pct = FAN_SPEED_MAX_PERCENTAGE;
                        AIM_LOG_ERROR("PSU %d - FAN %d failed!! status[%x], rmp[%d]", pid+1, fid+1, fan_info.status, fan_info.rpm);

                        goto set_temp;
                    }
                }
            }
        }
    }

    /* Get current temperature */
    if (onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_SA56004),        &thermal[0]) != ONLP_STATUS_OK ||
        onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_SA56004),  &thermal[1]) != ONLP_STATUS_OK ||
        onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_TMP75),  &thermal[2]) != ONLP_STATUS_OK ||
        onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_TMP75), &thermal[3]) != ONLP_STATUS_OK ||
        onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_TMP75), &thermal[4]) != ONLP_STATUS_OK ||
        onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(THERMAL_1_ON_LM95245), &thermal[5]) != ONLP_STATUS_OK ||
        onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(THERMAL_2_ON_LM95245), &thermal[6]) != ONLP_STATUS_OK ||
        onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(THERMAL_3_ON_LM95245),       &thermal[7]) != ONLP_STATUS_OK ||
        onlp_thermali_info_get(ONLP_THERMAL_ID_CREATE(THERMAL_4_ON_LM95245),       &thermal[8]) != ONLP_STATUS_OK
       )
    {
            /* Setting all fans speed to maximum */
            new_pct = FAN_SPEED_MAX_PERCENTAGE;
            AIM_LOG_ERROR("Unable to read thermal status");

	    goto set_temp;
    }

    // M.2 temperature
    thermal[CHASSIS_THERMAL_COUNT].mcelsius = get_nvme_temp(NVME_DEV)*100;
    thermal[CHASSIS_THERMAL_COUNT].thresholds.warning = 85000;
    thermal[CHASSIS_THERMAL_COUNT].thresholds.error = 90000;
    thermal[CHASSIS_THERMAL_COUNT].thresholds.shutdown = 95000;

#ifdef THERMAL_VERSION
    // print current time
    time(&timep);
    printf("\n%s",ctime(&timep));
#endif

    // The actual temperature is thermal_temp/1000
    // calculate fan speed by each Ka/100*temp+Kb/10
    for (i = 0; i < CHASSIS_THERMAL_COUNT+1 ; i++)
    {
	thermal_temp = thermal[i].mcelsius;
        temp_pct = (thermal_temp * Ka[i])/100000 + (Kb[i]/10);

        // add for thermal test, manual add offset to specific sensor (fan_index)
	if ( i == fan_index )
	{
	    thermal_temp += fan_offset;
	}

	// if temperature larger than warning threshold, set fan speed to maximun
	if ((i <= CHASSIS_THERMAL_COUNT) && (thermal_temp >= thermal[i].thresholds.warning))
	{
	    highest_pct = FAN_SPEED_MAX_PERCENTAGE;
#ifndef THERMAL_VERSION
	    AIM_LOG_ERROR("[WARNING] sensor [%d] temperature[%d] exceed warning temperature [%d]\n", i, thermal_temp, thermal[i].thresholds.warning);
#endif
	    //break;
	}

	if (temp_pct > highest_pct)
        {
            highest_pct = temp_pct;
        }

	if (thermal_temp > highest_temp)
	{
	    highest_temp = thermal_temp;
	}

#ifdef THERMAL_VERSION
         printf("[FAN_LOG]:sensors[%2d] temp[%3d.%3d], threshold[%3d/%3d/%3d], [Ka:%2d.%2d] [Kb:%4d.%d], speed[%4d], highest[%3d]\n", i, thermal_temp/1000, thermal_temp%1000, thermal[i].thresholds.warning/1000, thermal[i].thresholds.error/1000, thermal[i].thresholds.shutdown/1000, Ka[i]/100, Ka[i]%100, Kb[i]/10, abs(Kb[i]%10), temp_pct, highest_pct);
#endif
    }

    // read cfp2 temperature
    for (port = 0; port < CFP2_LED_COUNT; port++)
    {
	//rv = cfp_is_present(port);

	// cfp2 not present or incorrect
	//if(rv <= 0) {
	//    continue;
        //}

        // 0xB02F read temperature of CFP2 
        /*
	rv = onlp_mdio_read_c45(port, 0x9B07, &data16);
	printf ("[CFP2 DEBUG] Read CFP2[%d] 0x9B07: %x, rv=%x\n", port, data16, rv);

        rv = onlp_mdio_read_c45(port, 0x9B09, &data16);
        printf ("[CFP2 DEBUG] Read CFP2[%d] 0x9B09: %x, rv=%x\n", port, data16, rv);

        rv = onlp_mdio_read_c45(port, 0x9B0A, &data16);
        printf ("[CFP2 DEBUG] Read CFP2[%d] 0x9B0A: %x, rv=%x\n", port, data16, rv);

        rv = onlp_mdio_read_c45(port, 0x98A2, &data16);
        printf ("[CFP2 DEBUG] Read CFP2[%d] 098A2: %x, rv=%x\n", port, data16, rv);
        */
        rv = onlp_mdio_read_c45(port, 0xB02F, &data16);
        //printf ("[CFP2 DEBUG] Read CFP2[%d] 0xB02F: [%x], rv=[%x]\n", port, data16, rv);


	if ((rv == ONLP_STATUS_E_INVALID) || (data16 == 0xffff)) {
#ifdef THERMAL_VERSION		
	    qsfp_temp = 0;
#else
	    continue;
#endif
	}
	else
	{
            // cfp2 temperature = (data[1]*256+data[0])/256
	    qsfp_temp = data16*1000/256;

	    // add for thermal test, manual add offset to specific sensor (fan_index)
            if ( i == (fan_index-CHASSIS_THERMAL_COUNT) )
            {
                qsfp_temp += fan_offset;
            }
	}

        temp_pct = (qsfp_temp * Ka[i])/(100000) + (Kb[i]/10);

	// if temperature larger than warning threshold (cfp2/qsfp28 is 90 degree), set fan speed to maximun
	if ((qsfp_temp >= CFP2_WARNING_THRESHOLD) || (temp_pct > FAN_SPEED_MAX_PERCENTAGE))
	{
	    highest_pct = FAN_SPEED_MAX_PERCENTAGE;
#ifndef THERMAL_VERSION
	    AIM_LOG_ERROR("[WARNING] sensor cfp2 port[%d] temperature[%d] exceed warning temperature [%d]\n", port, qsfp_temp, CFP2_WARNING_THRESHOLD);
#endif
	    //break;
	}

	if (temp_pct > highest_pct)
        {
            highest_pct = temp_pct;
        }

	if (thermal_temp > highest_temp)
	{
	    highest_temp = thermal_temp;
	}

#ifdef THERMAL_VERSION
         printf("[FAN_LOG]:   CFP2[%2d] temp[%3d.%3d], threshold[%3d/%3d/%3d], [Ka:%2d.%2d] [Kb:%4d.%d], speed[%4d], highest[%3d]\n", port, qsfp_temp/1000, qsfp_temp%1000, CFP2_WARNING_THRESHOLD/1000, CFP2_ERROR_THRESHOLD/1000, CFP2_SHUTDOWN_THRESHOLD/1000, Ka[10]/100, Ka[10]%100, Kb[10]/10, abs(Kb[10]%10), temp_pct, highest_pct);
#endif
    }

    // Ka/Kb need take next index for QSFP28
    i++;

    // read qsfp28 temperature
    for (port = 1; port <= QSFP28_LED_COUNT; port++)
    {
	rv = onlp_sfp_is_present(port);

	// qsfp28 not present or incorrect
	if(rv <= 0) {
#ifdef THERMAL_VERSION
	    qsfp_temp = 0;
#else
 	    continue;
#endif
        }

        rv = onlp_sfpi_eeprom_read(port, data);

	if (rv < 0) {
#ifdef THERMAL_VERSION
            qsfp_temp = 0;
#else
 	    continue;
#endif
	}
	else
	{

            // qsfp28 temperature = (data[22]*256+data[23])/256
	    qsfp_temp = (data[22]*256+data[23])*1000/256;

	    // add for thermal test, manual add offset to specific sensor (fan_index)
            if ( i == (fan_index-CHASSIS_THERMAL_COUNT-CFP2_LED_COUNT) )
            {
                qsfp_temp += fan_offset;
            }
	}

        temp_pct = (qsfp_temp*Ka[i])/(100000) + (Kb[i]/10);

	// if temperature larger than warning threshold (qsfp28 is 90 degree), set fan speed to maximun
	if (qsfp_temp >= QSFP28_WARNING_THRESHOLD)
	{
	    highest_pct = FAN_SPEED_MAX_PERCENTAGE;
#ifndef THERMAL_VERSION
	    AIM_LOG_ERROR("[WARNING] sensor qsfp28 port[%d] temperature[%d] exceed warning temperature [%d]\n", port, qsfp_temp, QSFP28_WARNING_THRESHOLD);
#endif
	    //break;
	}

	if (temp_pct > highest_pct)
        {
            highest_pct = temp_pct;
        }

	if (thermal_temp > highest_temp)
	{
	    highest_temp = thermal_temp;
	}

#ifdef THERMAL_VERSION
         printf("[FAN_LOG]: QSFP28[%2d] temp[%3d.%3d], threshold[%3d/%3d/%3d], [Ka:%2d.%2d] [Kb:%4d.%d], speed[%4d], highest[%3d]\n", port, qsfp_temp/1000, qsfp_temp%1000, QSFP28_WARNING_THRESHOLD/1000, QSFP28_ERROR_THRESHOLD/1000, QSFP28_SHUTDOWN_THRESHOLD/1000, Ka[i]/100, Ka[i]%100, Kb[i]/10, abs(Kb[i]%10), temp_pct, highest_pct);
#endif
    }

    // step the fan speed by 5% 
    new_pct = (highest_pct / 5) *5;

    if ( new_pct >= FAN_SPEED_MAX_PERCENTAGE )
	new_pct = FAN_SPEED_MAX_PERCENTAGE;

    /* Default to full dump */
    //onlp_platform_dump(&aim_pvs_stdout,
    //                   ONLP_OID_DUMP_RECURSE | ONLP_OID_DUMP_EVEN_IF_ABSENT);
    //onlp_platform_dump(&aim_pvs_stdout, ONLP_OID_TYPE_THERMAL);

set_temp:

#ifdef THERMAL_VERSION
    printf("[FAN_CONTROL] highest temp: %d; FAN PWM: %d\n", highest_temp, new_pct);
#endif

    if ( new_pct < old_pct )
    {
	fan_counter++;
	
	if ( fan_counter <= FAN_SLOWDOWN_COUNTER )
	{
	    goto exit;
	}
	else
	{
	    old_pct = new_pct;
	    fan_counter = 0;
	}
    }
    else
    {
	fan_counter = 0;
	if ( new_pct > old_pct)
	   old_pct = new_pct;
	else // fan speed not changed
	   goto exit;
    }

    onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(FAN1_ON_PSU1) , old_pct);
    onlp_fani_percentage_set(ONLP_FAN_ID_CREATE(FAN1_ON_PSU2) , old_pct);

exit:
    return ONLP_STATUS_OK;
}

int
onlp_sysi_platform_manage_leds(void)
{
    int i = 0, rc = ONLP_STATUS_OK;
    int pid, fid;
    onlp_fan_info_t fan_info;
    onlp_psu_info_t psu_info[NUM_OF_PSU_ON_MAIN_BROAD];
    onlp_led_mode_t psu_new_mode = ONLP_LED_MODE_GREEN;
    onlp_led_mode_t sys_new_mode = ONLP_LED_MODE_GREEN;
    onlp_led_mode_t qsfp_new_mode = ONLP_LED_MODE_GREEN;
    onlp_led_mode_t piu_new_mode = ONLP_LED_MODE_BLUE;

    // PSU Status 
    for(pid = 0; pid < NUM_OF_PSU_ON_MAIN_BROAD; pid++)
    {
        rc = onlp_psui_info_get(ONLP_PSU_ID_CREATE(pid+1), &psu_info[i]);

        if ( (rc != ONLP_STATUS_OK) || !(psu_info[i].status & ONLP_PSU_STATUS_PRESENT) )
        {
            psu_new_mode = ONLP_LED_MODE_RED;
        }
    }

    // FAN status
    for(pid = 0; pid < NUM_OF_PSU_ON_MAIN_BROAD; pid++)
    {
	for (fid = 0; fid < NUM_OF_FAN_ON_PSU_BROAD; fid++) 
	{
	    if (psu_info[pid].status & ONLP_PSU_STATUS_PRESENT)
	    {
                rc = onlp_fani_info_get(ONLP_FAN_ID_CREATE(pid*NUM_OF_FAN_ON_PSU_BROAD+fid+1), &fan_info);

                if ( (rc != ONLP_STATUS_OK) || !(fan_info.status & ONLP_FAN_STATUS_PRESENT) )
                {
                    psu_new_mode = ONLP_LED_MODE_RED_BLINKING;
                    goto SET_LED;
                }
	    }
	}
    }


SET_LED:
    /// SYS LED
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_SYS), sys_new_mode);
    // PSU LED
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_PSU), psu_new_mode);

    // SFP LED


    for ( i = 0; i < QSFP28_LED_COUNT; i++ )
    {
        if ((rc = onlp_sfpi_is_present (i+1)))
        {
            // not suppot speed status, set to BLUE (400G)
            qsfp_new_mode = ONLP_LED_MODE_GREEN;
        }
        else
        {
            qsfp_new_mode = ONLP_LED_MODE_OFF;
        }

        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_QSFP1+i), qsfp_new_mode);
    }

    for ( i = 0; i < CFP2_LED_COUNT; i++ )
    {
        rc = get_module_status (i+1);

	if ( (rc & (ONLP_MODULE_STATUS_PIU_DCO_PRESENT | ONLP_MODULE_STATUS_PIU_CFP2_PRESENT)) )
        {
            // not suppot speed status, set to BLUE (400G)
            piu_new_mode = ONLP_LED_MODE_BLUE;
        }
        else
        {
            piu_new_mode = ONLP_LED_MODE_OFF;
        }

        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_CFP1+i), piu_new_mode);
    }

    return ONLP_STATUS_OK;
}

//#define	LED_UNIT_TEST	1

#ifdef LED_UNIT_TEST
int
onlp_sysi_platform_manage_leds(void)
{
    int i = 0;




    // unit test to set LED

    /* GGGG    
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_SYS), ONLP_LED_MODE_GREEN);
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_PSU), ONLP_LED_MODE_GREEN);
    for ( i = 0; i < QSFP28_LED_COUNT; i++ )
    {
        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_QSFP1+i), ONLP_LED_MODE_GREEN);
    }

    for ( i = 0; i < CFP2_LED_COUNT; i++ )
    {
        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_CFP1+i), ONLP_LED_MODE_GREEN);
    }
    */

    /* GGGG BLINKING
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_SYS), ONLP_LED_MODE_GREEN_BLINKING);
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_PSU), ONLP_LED_MODE_GREEN_BLINKING);
    for ( i = 0; i < QSFP28_LED_COUNT; i++ )
    {
        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_QSFP1+i), ONLP_LED_MODE_GREEN_BLINKING);
    }

    for ( i = 0; i < CFP2_LED_COUNT; i++ )
    {
        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_CFP1+i), ONLP_LED_MODE_GREEN_BLINKING);
    }
    */

    /* RRYB
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_SYS), ONLP_LED_MODE_RED);
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_PSU), ONLP_LED_MODE_RED);
    for ( i = 0; i < QSFP28_LED_COUNT; i++ )
    {
        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_QSFP1+i), ONLP_LED_MODE_YELLOW);
    }

    for ( i = 0; i < CFP2_LED_COUNT; i++ )
    {
        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_CFP1+i), ONLP_LED_MODE_BLUE);
    }
    */

    /* RRYB BLINKING 
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_SYS), ONLP_LED_MODE_RED_BLINKING);
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_PSU), ONLP_LED_MODE_RED_BLINKING);
    for ( i = 0; i < QSFP28_LED_COUNT; i++ )
    {
        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_QSFP1+i), ONLP_LED_MODE_YELLOW_BLINKING);
    }

    for ( i = 0; i < CFP2_LED_COUNT; i++ )
    {
        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_CFP1+i), ONLP_LED_MODE_BLUE_BLINKING);
    }
    */

    /* All OFF */
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_SYS), ONLP_LED_MODE_OFF);
    onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_PSU), ONLP_LED_MODE_OFF);
    for ( i = 0; i < QSFP28_LED_COUNT; i++ )
    {
        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_QSFP1+i), ONLP_LED_MODE_OFF);
    }

    for ( i = 0; i < CFP2_LED_COUNT; i++ )
    {
        onlp_ledi_mode_set(ONLP_LED_ID_CREATE(LED_CFP1+i), ONLP_LED_MODE_OFF);
    }
    /**/

    return ONLP_STATUS_OK;
}
#endif


