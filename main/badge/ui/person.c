#include "person.h"

static lv_obj_t *person_name;
static lv_obj_t *person_organization;
static lv_obj_t *person_job;
static lv_obj_t *person_message;

void ui_set_person(bool secret) {
  if (secret == true) {
    lv_label_set_text(person_name, badge_obj.secret_name);
    lv_label_set_text(person_organization, badge_obj.secret_organization);
    lv_label_set_text(person_job, badge_obj.secret_job);
    lv_label_set_text(person_message, badge_obj.secret_message);
  } else {
    lv_label_set_text(person_name, badge_obj.person_name);
    lv_label_set_text(person_organization, badge_obj.person_organization);
    lv_label_set_text(person_job, badge_obj.person_job);
    lv_label_set_text(person_message, badge_obj.person_message);
  }
}

lv_obj_t *ui_screen_person_init() {
  /* Styling */
  static lv_style_t style_name;
  lv_style_init(&style_name);
  lv_style_set_text_font(&style_name, &lv_font_montserrat_22);
  lv_style_set_text_decor(&style_name, LV_TEXT_DECOR_UNDERLINE);

  lv_style_t style_organization_job;
  lv_style_init(&style_organization_job);
  lv_style_set_text_font(&style_organization_job, &lv_font_montserrat_18);

  /* Screen and labels creation */
  lv_obj_t *screen_person = lv_obj_create(NULL);
  lv_obj_clear_flag(screen_person, LV_OBJ_FLAG_SCROLLABLE);

  person_name = lv_label_create(screen_person); /*Used as a base label*/
  lv_label_set_long_mode(person_name,
                         LV_LABEL_LONG_WRAP); /*Break the long lines*/
  lv_label_set_recolor(person_name,
                       true); /*Enable re-coloring by commands in the text*/
  lv_obj_set_style_text_align(person_name, LV_TEXT_ALIGN_CENTER,
                              0); /*Center aligned lines*/
  lv_obj_set_width(person_name, NAME_LABEL_SIZE);
  lv_obj_align(person_name, LV_ALIGN_CENTER, 0, -60);

  person_organization = lv_label_create(screen_person);
  lv_obj_align(person_organization, LV_ALIGN_CENTER, 0, -30);

  person_job = lv_label_create(screen_person);
  lv_obj_align(person_job, LV_ALIGN_CENTER, 0, 0);

  person_message = lv_label_create(screen_person);
  lv_obj_align(person_message, LV_ALIGN_CENTER, 0, 60);

  /* Setting styles */
  lv_obj_add_style(person_name, &style_name, LV_PART_MAIN);
  lv_obj_add_style(person_organization, &style_organization_job, LV_PART_MAIN);
  lv_obj_add_style(person_job, &style_organization_job, LV_PART_MAIN);
  /* Stetting texts */
  ui_set_person(false);

  return (screen_person);
}

void person_button_up() { ui_set_person(true); }

void person_button_down() { ui_set_person(false); }