#ifndef _EVENT_H
#define _EVENT_H
#include <lvgl.h>

void ui_event_load_schedule();
lv_obj_t *ui_event_init();
void ui_event_button_up();
void ui_event_button_down();

#endif