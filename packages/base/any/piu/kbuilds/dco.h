#include <linux/i2c.h>

#define DCO_PIU_CFP2_OFFSET 0x00300000

#define MCU_DEV_ADDR 0x00

#define DCO_DEV_ADDR 0x30
#define QSFP28_1_DEV_ADDR_OFFSET 0x00310000
#define QSFP28_2_DEV_ADDR_OFFSET 0x00320000
#define QSFP28_EEPROM_SIZE 256

#define RETIMER_1_DEV_ADDR 0x40
#define RETIMER_2_DEV_ADDR 0x41
#define EEPROM_DEV_ADDR 0x50



int dco_piu_i2c_read(struct i2c_client *client, uint32_t addr, uint32_t *value);
int dco_piu_i2c_write(struct i2c_client *client, uint32_t addr, uint32_t value);
int dco_cfp2_exists(struct i2c_client *client);
