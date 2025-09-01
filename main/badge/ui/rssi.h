#ifndef _RSSI_H
#define _RSSI_H

#include "lvgl.h"
#include "../badge.h"

extern lv_timer_t *rssi_timer_handle;
extern lv_obj_t *screen_rssi;

lv_obj_t *ui_screen_rssi_init();

#endif