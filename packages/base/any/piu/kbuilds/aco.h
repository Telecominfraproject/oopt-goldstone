#include <linux/i2c.h>

#define ACO_PIU_THERMAL_ENABLED 0x23c0
#define ACO_PIU_THERMAL_VALUE   0x23c4

#define ACO_PIU_CFP2_OFFSET 0x40000000

#define ACO_PIU_FPGA_VERSION 0x30090000

int aco_piu_i2c_read(struct i2c_client *client, uint32_t addr, uint32_t *value);
int aco_piu_i2c_write(struct i2c_client *client, uint32_t addr, uint32_t value);
int aco_cfp2_exists(struct i2c_client *client);
