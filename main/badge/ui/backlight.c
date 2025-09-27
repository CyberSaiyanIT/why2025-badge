#include "backlight.h"
#include "ui.h"
#include "../led.h"
#include "../common/i2c.h"
static uint32_t last_trigger = -1;
static lv_timer_t *backlight_timer_handle;

/*
 * Update the screen backlight status
 * returns status of backlight (true if backlight off)
 */

static void ui_backlight_set(uint8_t brigtness) {
  i2c_write_register(AW9523B_handle, 0x20, brigtness);
  i2c_write_register(AW9523B_handle, 0x21, brigtness);
  i2c_write_register(AW9523B_handle, 0x22, brigtness);
  i2c_write_register(AW9523B_handle, 0x23, brigtness);
}

void ui_backlight_set_max() { ui_backlight_set(badge_obj.brightness_max); }
void ui_backlight_set_mid() { ui_backlight_set(badge_obj.brightness_mid); }
void ui_backlight_set_off() { ui_backlight_set(badge_obj.brightness_off); }

bool ui_backlight_update(bool trigger) {
  uint32_t span = lv_tick_get() - last_trigger;

  if (trigger) {
    ui_backlight_set_max();
    last_trigger = lv_tick_get();

    ui_resume_current_screen();
  } else {
    if (span > BRIGHT_OFF_TIMEOUT_MS) {
      ui_backlight_set_off();
      ui_pause_current_screen();
    } else if (span > BRIGHT_MID_TIMEOUT_MS)
      ui_backlight_set_mid();
  }

  /* Avoid doing action when backlight off */
  if (span > BRIGHT_OFF_TIMEOUT_MS) {
    return true;
  }
  return false;
}

static void ui_backlight_timer_handler(lv_timer_t *arg) { ui_backlight_update(false); }

void ui_backlight_init() {
  backlight_timer_handle = lv_timer_create(ui_backlight_timer_handler, 1000, NULL);
  lv_timer_resume(backlight_timer_handle);
}