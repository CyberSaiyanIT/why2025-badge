#ifndef _RADAR_H
#define _RADAR_H

#include "lvgl.h"
#include "../badge.h"

lv_timer_t *radar_timer_handle;
lv_obj_t *screen_radar;

lv_obj_t *ui_screen_radar_init();

#endif