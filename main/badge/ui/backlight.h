#ifndef _BACKLIGHT_H
#define _BACKLIGHT_H

#include "lvgl.h"

// Brightness constants - now configured via default.json
// Legacy values: SCREEN_BRIGHT_MAX=96, SCREEN_BRIGHT_MID=32,
// SCREEN_BRIGHT_OFF=0 New default values: brightness_max=255,
// brightness_mid=200, brightness_off=0 #define SCREEN_BRIGHT_MAX 96 #define
// SCREEN_BRIGHT_MID 32 #define SCREEN_BRIGHT_OFF 0

#define BRIGHT_MID_TIMEOUT_MS 5000
#define BRIGHT_OFF_TIMEOUT_MS 15000


lv_timer_t *backlight_timer_handle;

bool ui_update_backlight(bool trigger);
void backlight_init();
#endif