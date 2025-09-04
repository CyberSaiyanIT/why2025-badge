#include "i2c.h"

void i2c_register_write(uint8_t addr, uint8_t reg_addr, uint8_t data) {
  uint8_t write_buf[2] = {reg_addr, data};
  i2c_master_write_to_device(0, addr, write_buf, sizeof(write_buf),
                             I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

 void i2c_master_init() {
  int i2c_master_port = I2C_MASTER_NUM;

  i2c_config_t conf = {
      .mode             = I2C_MODE_MASTER,
      .sda_io_num       = I2C_MASTER_SDA_IO,
      .scl_io_num       = I2C_MASTER_SCL_IO,
      .sda_pullup_en    = GPIO_PULLUP_ENABLE,
      .scl_pullup_en    = GPIO_PULLUP_ENABLE,
      .master.clk_speed = I2C_MASTER_FREQ_HZ,
  };

  i2c_param_config(i2c_master_port, &conf);
  i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}