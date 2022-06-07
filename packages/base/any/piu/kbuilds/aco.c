#include <linux/delay.h>

#include "aco.h"

#define I2C_ADDRESS_SIZE 4

static int I2C_ADDRESS_ADDRESS[] = {0x10, 0x11, 0x12, 0x13};
static int I2C_DATA_ADDRESS[] = {0x20, 0x21, 0x22, 0x23};

#define I2C_TRIGGER_ADDRESS  0x01
#define I2C_TRIGGER_WRITE 0x01
#define I2C_TRIGGER_READ 0x02

#define I2C_READY_ADDRESS 0x02
#define I2C_READY_VALUE 0x00

#define I2C_POLL_TIMEOUT 50
#define I2C_POLL_INTERVAL_US 100

static int piu_i2c_wait_ready(struct i2c_client *client) {
    int i, ret;
    for (i = 0; i < I2C_POLL_TIMEOUT; i++) {
        ret = i2c_smbus_read_byte_data(client, I2C_READY_ADDRESS);
        if ( ret < 0 ){
            return ret;
        }
        if ( ret == I2C_READY_VALUE ) {
            return 0;
        }
        msleep(I2C_POLL_INTERVAL_US);
    }
    return -1;
}

int aco_piu_i2c_read(struct i2c_client *client, uint32_t addr, uint32_t *value) {
    int i, ret;
    if ( client == NULL || value == NULL ) {
        return -1;
    }
    *value = 0;
    for (i = 0; i < I2C_ADDRESS_SIZE; i++) {
        if ((ret = i2c_smbus_write_byte_data(client, I2C_ADDRESS_ADDRESS[i], (addr >> ((I2C_ADDRESS_SIZE - 1 - i)*8)) & 0xff)) < 0) {
            return ret;
        }
    }
    if ((ret = i2c_smbus_write_byte_data(client, I2C_TRIGGER_ADDRESS, I2C_TRIGGER_READ)) < 0) {
        return ret;
    }
    if ( piu_i2c_wait_ready(client) < 0 ){
        return -1;
    }
    for (i = 0; i < I2C_ADDRESS_SIZE; i++) {
        ret = i2c_smbus_read_byte_data(client, I2C_DATA_ADDRESS[i]);
        if ( ret < 0 ){
            return -1;
        }
        *value |= (ret << ((I2C_ADDRESS_SIZE - 1 - i)*8));
    }
    return 0;
}

int aco_piu_i2c_write(struct i2c_client *client, uint32_t addr, uint32_t value) {
    int i, ret;
    if ( client == NULL ) {
        return -1;
    }
    for (i = 0; i < I2C_ADDRESS_SIZE; i++) {
        if ((ret = i2c_smbus_write_byte_data(client, I2C_ADDRESS_ADDRESS[i], (addr >> ((I2C_ADDRESS_SIZE - 1 - i)*8)) & 0xff)) < 0) {
            return ret;
        }
    }
    for (i = 0; i < I2C_ADDRESS_SIZE; i++) {
        if ((ret = i2c_smbus_write_byte_data(client, I2C_DATA_ADDRESS[i], (value >> ((I2C_ADDRESS_SIZE - 1 - i)*8)) & 0xff)) < 0) {
            return ret;
        }
    }
    if ((ret = i2c_smbus_write_byte_data(client, I2C_TRIGGER_ADDRESS, I2C_TRIGGER_WRITE)) < 0) {
        return ret;
    }
    return piu_i2c_wait_ready(client);
}

int aco_cfp2_exists(struct i2c_client *client) {
    int ret;
    uint32_t value;
    ret = aco_piu_i2c_read(client, 0x30091004, &value);
    if ( ret ) {
        return 0;
    }
    if ( (value & 0x1) == 0 ) {
        return 1;
    }
    return 0;
}
