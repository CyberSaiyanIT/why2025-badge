#ifndef _RSSI_H
#define _RSSI_H

#include <lvgl.h>
#include "../badge.h"

lv_obj_t *ui_rssi_init();
void ui_rssi_load();
void ui_rssi_unload();
#endif