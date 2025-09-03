#ifndef _PERSON_H
#define _PERSON_H

#include <lvgl.h>
#include "../badge.h"

void ui_set_person(bool secret);
lv_obj_t *ui_screen_person_init();
void person_button_up();
void person_button_down();

#endif