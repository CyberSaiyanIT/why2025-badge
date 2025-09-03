#ifndef _EVENT_H
#define _EVENT_H

#include <stdio.h>
#include <stdlib.h>

#include <lvgl.h>
#include "../badge.h"

void ui_event_load();
lv_obj_t *ui_screen_event_init();
void event_button_up();
void event_button_down();

#endif