#ifndef _UI_H
#define _UI_H

#include <stdio.h>
#include <stdlib.h>

#include <lvgl.h>

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

#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB888))

// Using SPI2 in the example
#define LCD_HOST SPI2_HOST

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your
/// LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define LCD_PIXEL_CLOCK_HZ (40 * 1000 * 1000)

#define PIN_NUM_SCLK     6
#define PIN_NUM_MOSI     7
#define PIN_NUM_MISO     2
#define PIN_NUM_LCD_DC   4
#define PIN_NUM_LCD_RST  3
#define PIN_NUM_LCD_CS   10
#define PIN_NUM_BK_LIGHT -1

// The pixel number in horizontal and vertical
#define LCD_H_RES 320
#define LCD_V_RES 240
// Bit number used to represent command and parameter
#define LCD_CMD_BITS        8
#define LCD_PARAM_BITS      8
#define LVGL_DRAW_BUF_LINES 20  // number of display lines in each draw buffer
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

extern lv_obj_t *screens[NUM_SCREENS];
extern int8_t current_screen;

void scroll_up();
void scroll_down();
void ui_switch_page_up();
void ui_switch_page_down();
void ui_task(void *);

// void ui_switch_page_up();
// void ui_switch_page_down();
#endif