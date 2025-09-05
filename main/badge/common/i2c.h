#ifndef _I2C_H
#define _I2C_H
#include "driver/i2c_master.h"
#include <stdint.h>

#define I2C_MASTER_SCL_IO     0
#define I2C_MASTER_SDA_IO     1
#define I2C_MASTER_NUM        0
#define I2C_MASTER_FREQ_HZ    400000
#define I2C_MASTER_TIMEOUT_MS 1000

#define AW9523B_ADDRESS (0x5a)
#define TSC2007_ADDRESS (0x48)

extern i2c_master_dev_handle_t AW9523B_handle;
extern i2c_master_dev_handle_t TSC2007_handle;

esp_err_t i2c_register_write(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data);
esp_err_t i2c_register_write_read(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len);
void new_i2c_device(uint16_t device_address, i2c_master_dev_handle_t *dev_handle);
void i2c_master_init();

#endif