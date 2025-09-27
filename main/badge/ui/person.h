#ifndef _PERSON_H
#define _PERSON_H

#include <lvgl.h>
#include "../badge.h"

void ui_person_set_secret(bool secret);
lv_obj_t *ui_person_init();
void ui_person_load();
void ui_person_button_up();
void ui_person_button_down();

#endif