#ifndef _UI_H
#define _UI_H

#include <stdio.h>
#include <stdlib.h>

#include <lvgl.h>

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

#define NAME_LABEL_SIZE 300
#define SCROLL_UP 80
#define SCROLL_DOWN -80

extern lv_obj_t *screens[NUM_SCREENS];
extern int8_t current_screen;


void scroll(int dy);
void scroll_up();
void scroll_down();
void ui_switch_page_up();
void ui_switch_page_down();
void ui_task(void *);

#endif