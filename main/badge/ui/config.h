#ifndef _CONFIG_H
#define _CONFIG_H

#include <lvgl.h>

enum screen_order {
  SCREEN_SPLASH,
  SCREEN_PERSON,
  SCREEN_SOCIALENERGY,
  SCREEN_DICE,
  SCREEN_EVENT,
  SCREEN_RADAR,
  SCREEN_RSSI,
  SCREEN_ADMIN,
  SCREEN_SNAKE,
  NUM_SCREENS
};

#define FIRST_SCREEN SCREEN_SPLASH
typedef struct {
  uint8_t screen_id;
  lv_obj_t* (*screen_init)();
  void (*load_screen)();
  void (*unload_screen)();
  void (*resume_screen)();
  void (*pause_screen)();
  void (*button_up)();
  void (*button_down)();
} screen_config_t;

extern screen_config_t screen_config[];
uint8_t ui_config_get_nb_screens();
#endif
