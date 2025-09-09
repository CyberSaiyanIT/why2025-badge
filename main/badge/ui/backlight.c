#include "backlight.h"
#include "../led.h"

static uint32_t last_trigger = -1;

/*
 * Update the screen backlight status
 * returns status of backlight (true if backlight off)
 */

void restore_current_timer();
void pause_current_timer();

bool ui_update_backlight(bool trigger) {
  uint32_t span = lv_tick_get() - last_trigger;

  if (trigger) {
    set_screen_led_backlight(badge_obj.brightness_max);
    last_trigger = lv_tick_get();

    restore_current_timer();
  } else {
    if (span > BRIGHT_OFF_TIMEOUT_MS) {
      set_screen_led_backlight(badge_obj.brightness_off);
      pause_current_timer();
    } else if (span > BRIGHT_MID_TIMEOUT_MS) {
      set_screen_led_backlight(badge_obj.brightness_mid);
    }
  }

  /* Avoid doing action when backlight off */
  if (span > BRIGHT_OFF_TIMEOUT_MS) {
    return true;
  }
  return false;
}

static void ui_backlight_timer(lv_timer_t *arg) { ui_update_backlight(false); }

void backlight_init() {
  backlight_timer_handle = lv_timer_create(ui_backlight_timer, 1000, NULL);
  lv_timer_resume(backlight_timer_handle);
}