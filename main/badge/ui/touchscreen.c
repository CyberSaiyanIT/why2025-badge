#include "touchscreen.h"
#include <lvgl.h>
#include "../common/i2c.h"

#include "lcd.h"
#include "backlight.h"

static uint16_t ui_touchscreen_read_cmd(const tsc2007_function func) {
  uint8_t buf[2];

  uint8_t cmd = (uint8_t)func << 4;
  cmd |= (uint8_t)ADON_IRQOFF << 2;
  cmd |= (uint8_t)ADC_12BIT << 1;

  ESP_ERROR_CHECK(i2c_write_read_register(TSC2007_handle, cmd, buf, sizeof(buf)));
  return (uint16_t)((buf[0] << 4) | (buf[1] >> 4));
}

uint16_t ui_touchscreen_read_x() { return ui_touchscreen_read_cmd(MEASURE_X); }
uint16_t ui_touchscreen_read_y() { return ui_touchscreen_read_cmd(MEASURE_Y); }
uint16_t ui_touchscreen_read_z1() { return ui_touchscreen_read_cmd(MEASURE_Z1); }
uint16_t ui_touchscreen_read_z2() { return ui_touchscreen_read_cmd(MEASURE_Z2); }

static bool touchscreen_i2c_read(point_t *point) {
  uint16_t z1 = ui_touchscreen_read_z1();
  uint16_t z2 = ui_touchscreen_read_z2();

  // No touch
  if (z1 >= RESOLLUTION_12BIT || z2 >= RESOLLUTION_12BIT)
    return false;

  uint16_t x1 = ui_touchscreen_read_x();
  uint16_t y1 = ui_touchscreen_read_y();

  uint16_t x2 = ui_touchscreen_read_x();
  uint16_t y2 = ui_touchscreen_read_y();

  // Wrong reading
  if (abs((int32_t)x1 - (int32_t)x2) > 100)
    return false;
  if (abs((int32_t)y1 - (int32_t)y2) > 100)
    return false;

  point->x = LCD_H_RES - (x1 * LCD_H_RES) / RESOLLUTION_12BIT;
  point->y = (y1 * LCD_V_RES) / RESOLLUTION_12BIT;
  point->z = z1;
  return true;
}

static void ui_touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data) {
  point_t touchscreen_point;

  if (touchscreen_i2c_read(&touchscreen_point)) {
    if (ui_backlight_update(true))
      return;
    data->point.x = touchscreen_point.x;
    data->point.y = touchscreen_point.y;
    data->state   = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void ui_touchscreen_init() {
  lv_indev_t *touchscreen_indev = lv_indev_create();           /* Create input device connected to Default Display. */
  lv_indev_set_type(touchscreen_indev, LV_INDEV_TYPE_POINTER); /* Touch pad is a pointer-like device. */
  lv_indev_set_read_cb(touchscreen_indev, ui_touchscreen_read);
}