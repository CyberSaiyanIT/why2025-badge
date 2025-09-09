#ifndef _UI_H
#define _UI_H

#include <stdio.h>
#include <stdlib.h>

#include "lvgl.h"
#include "lvgl_helpers.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "../badge.h"
#include "admin.h"
#include "event.h"
#include "person.h"
#include "radar.h"
#include "rssi.h"
#include "snake.h"
#include "socialenergy.h"
#include "splash.h"

#include "backlight.h"
#include "buttons.h"

enum screen_order {
  SCREEN_LOGO,
  SCREEN_PERSON,
  SCREEN_SOCIALENERGY,
  SCREEN_EVENT,
  SCREEN_RADAR,
  SCREEN_RSSI,
  SCREEN_ADMIN,
  SCREEN_SNAKE,
  NUM_SCREENS
};

lv_obj_t *screens[NUM_SCREENS];
int8_t current_screen;

void scroll_up();
void scroll_down();
void ui_switch_page_up();
void ui_switch_page_down();
void ui_task(void *);

// void ui_switch_page_up();
// void ui_switch_page_down();
#endif