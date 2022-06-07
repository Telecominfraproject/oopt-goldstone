#include <linux/delay.h>

#include "dco.h"

#define I2C_POLL_TIMEOUT 1
#define I2C_POLL_INTERVAL_MS 70

//for PIU-DCO MCU
static int m487_write_block(struct i2c_client *client, uint16_t size, uint8_t *values) {
    return i2c_smbus_write_i2c_block_data(client, values[0], (size-1), &values[1]);
}

//for PIU-DCO MCU
static int m487_read_block(struct i2c_client *client, uint16_t size, uint8_t *values) {
    return i2c_smbus_read_i2c_block_data(client, 0xff, size, values); //PIU-DCO MCU will ignore the 0xff command.
}

//for PIU-DCO MCU
static int read_from_mcu_device(struct i2c_client *client, uint8_t dev_addr, uint16_t addr, uint16_t *value)
{
    uint8_t buf[3];
    int ret = 0, retry = 0;
    int size = dev_addr == DCO_DEV_ADDR ? 3 : 2;

    /*write device address and register address*/
    buf[0] = dev_addr;
    buf[1] = (addr >> 8) & 0x00ff;
    buf[2] = addr & 0x00ff;

    ret = m487_write_block(client, 3, buf);
    if (ret) {
        return ret;
    }
    memset(buf, 0, 3);

    /*read status and data*/
    do {
        ret = m487_read_block(client, size, buf);
        if ((ret < 0) || (buf[0] != 0)) {
            printk("  retry %d, status=0x%x, ret %d\n", retry, buf[0], ret);
            msleep(I2C_POLL_INTERVAL_MS); //waiting for MCU to prepare data
        }
    } while (((ret < 0) || (buf[0] != 0)) && (retry++<I2C_POLL_TIMEOUT));

    if ( ret < 0 ) {
        return ret;
    } else if ( buf[0] != 0 ) {
        return -buf[0];
    }

    *value = (buf[2] << 8) | buf[1];

    return 0;
}

//for PIU-DCO MCU
static int write_to_mcu_device(struct i2c_client *client, uint8_t dev_addr, uint16_t addr, uint16_t value)
{
    int size_w;
    unsigned char data_w[16];

    data_w[0] = dev_addr;
    data_w[1] = (addr >> 8) & 0x00ff;
    data_w[2] = addr & 0x00ff;

    if ( dev_addr == DCO_DEV_ADDR ) {
        size_w = 5;
        data_w[3] = value & 0x00ff;
        data_w[4] = (value >> 8) & 0x00ff;
    } else {
        size_w = 4;
        data_w[3] = value & 0xff;
    }

    return m487_write_block(client, size_w, data_w);
}

int dco_piu_i2c_read(struct i2c_client *client, uint32_t addr, uint32_t *value) {
    int ret;
    uint16_t v = 0;
    ret = read_from_mcu_device(client, addr >> 16, addr & 0xffff, &v);
    *value = v;
    return ret;
}

int dco_piu_i2c_write(struct i2c_client *client, uint32_t addr, uint32_t value) {
    return write_to_mcu_device(client, addr >> 16, addr & 0xffff, value);
}

int dco_cfp2_exists(struct i2c_client *client) {
    int ret;
    uint32_t value;
    ret = dco_piu_i2c_read(client, 0x5, &value);
    if ( ret ) {
        return 0;
    }
    if ( (value & 0x8) == 0 ) {
        return 1;
    }
    return 0;
}
