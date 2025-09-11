#include "i2c.h"

#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"

static i2c_master_bus_handle_t bus_handle;

i2c_master_dev_handle_t AW9523B_handle;
i2c_master_dev_handle_t TSC2007_handle;

esp_err_t i2c_write_register(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data) {
  uint8_t write_buf[2] = {reg_addr, data};
  return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t i2c_write_read_register(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len) {
  return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static void i2c_new_device(uint16_t device_address, i2c_master_dev_handle_t *dev_handle) {
  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address  = device_address,
      .scl_speed_hz    = I2C_MASTER_FREQ_HZ,
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, dev_handle));
}

void i2c_master_init() {
  i2c_master_bus_config_t bus_config = {
      .i2c_port                     = I2C_MASTER_NUM,
      .sda_io_num                   = I2C_MASTER_SDA_IO,
      .scl_io_num                   = I2C_MASTER_SCL_IO,
      .clk_source                   = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt            = 7,
      .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
  i2c_new_device(AW9523B_ADDRESS, &AW9523B_handle);
  i2c_new_device(TSC2007_ADDRESS, &TSC2007_handle);
}