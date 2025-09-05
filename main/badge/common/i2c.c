#include "i2c.h"

#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"

static i2c_master_bus_handle_t bus_handle;

i2c_master_dev_handle_t AW9523B_handle;
i2c_master_dev_handle_t TSC2007_handle;

esp_err_t i2c_register_write(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data) {
  uint8_t write_buf[2] = {reg_addr, data};
  return i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

esp_err_t i2c_register_write_read(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len) {
  return i2c_master_transmit_receive(dev_handle, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

/*void i2c_register_read(uint8_t addr, uint8_t data) {
  i2c_master_receive(0, addr, read_buf, read_size,
                     I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}*/

/*void i2c_scanner() {
  ESP_LOGI(__FILE__, "Starting I2C scan...");

  int devices_found = 0;
  for (int address = 1; address < 127; address++) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 10 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    if (ret == ESP_OK) {
      ESP_LOGI(__FILE__, "I2C device found at address 0x%02x", address);
      devices_found++;
    } else if (ret == ESP_ERR_TIMEOUT) {
      ESP_LOGW(__FILE__, "Timeout at address 0x%02x", address);
    }
  }

  if (devices_found == 0) {
    ESP_LOGI(__FILE__, "No I2C devices found\n");
  } else {
    ESP_LOGI(__FILE__, "Scan complete. %d devices found.\n", devices_found);
  }
}*/

void new_i2c_device(uint16_t device_address, i2c_master_dev_handle_t *dev_handle) {
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
  new_i2c_device(AW9523B_ADDRESS, &AW9523B_handle);
  new_i2c_device(TSC2007_ADDRESS, &TSC2007_handle);
}