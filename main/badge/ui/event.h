#ifndef _EVENT_H
#define _EVENT_H
#include <lvgl.h>

void ui_event_load();
lv_obj_t *ui_screen_event_init();
void event_button_up();
void event_button_down();

#endif