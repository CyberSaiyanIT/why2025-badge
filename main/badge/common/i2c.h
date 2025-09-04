#ifndef _I2C_H
#define _I2C_H

#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO           0
#define I2C_MASTER_SDA_IO           1
#define I2C_MASTER_NUM              0
#define I2C_MASTER_FREQ_HZ          400000
#define I2C_MASTER_TIMEOUT_MS       1000

void i2c_register_write(uint8_t addr, uint8_t reg_addr, uint8_t data);
void i2c_master_init();
#endif