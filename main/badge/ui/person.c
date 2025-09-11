#include "person.h"
#include "ui.h"

static bool current_secret = false;
static lv_obj_t *person_name;
static lv_obj_t *person_organization;
static lv_obj_t *person_job;
static lv_obj_t *person_message;
static lv_obj_t *person_qr;

void ui_person_set_secret(bool secret) {
  current_secret   = secret;
  person_t *person = &badge_obj.person;
  if (secret == true)
    person = &badge_obj.secret;

  lv_label_set_text(person_name, person->name);
  lv_label_set_text(person_organization, person->organization);
  lv_label_set_text(person_job, person->job);
  lv_label_set_text(person_message, person->message);
  lv_qrcode_update(person_qr, person->url, strlen(person->url));
}

static void ui_person_toggle_secret_event(lv_event_t *event) {
  lv_obj_add_flag(person_qr, LV_OBJ_FLAG_HIDDEN);
  ui_person_set_secret(!current_secret);
}

static void ui_person_qrcode_event(lv_event_t *event) {
  if (lv_obj_has_flag(person_qr, LV_OBJ_FLAG_HIDDEN))
    lv_obj_remove_flag(person_qr, LV_OBJ_FLAG_HIDDEN);
  else
    lv_obj_add_flag(person_qr, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *ui_person_create_label(lv_obj_t *screen) {
  lv_obj_t *label = lv_label_create(screen);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);           /*Break the long lines*/
  lv_label_set_recolor(label, true);                           /*Enable re-coloring by commands in the text*/
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0); /*Center aligned lines*/
  lv_obj_set_width(label, NAME_LABEL_SIZE);
  return label;
}

lv_obj_t *ui_person_init() {
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

  person_name = ui_person_create_label(screen_person); /*Used as a base label*/
  lv_obj_align(person_name, LV_ALIGN_CENTER, 0, -60);

  person_organization = ui_person_create_label(screen_person);
  lv_obj_align(person_organization, LV_ALIGN_CENTER, 0, -30);

  person_job = ui_person_create_label(screen_person);
  lv_obj_align(person_job, LV_ALIGN_CENTER, 0, 0);

  person_message = ui_person_create_label(screen_person);
  lv_obj_align(person_message, LV_ALIGN_CENTER, 0, 60);

  person_qr = lv_qrcode_create(screen_person);
  lv_obj_add_flag(person_qr, LV_OBJ_FLAG_HIDDEN);
  lv_qrcode_set_size(person_qr, 200);
  lv_obj_center(person_qr);

  /* Setting styles */
  lv_obj_add_style(person_name, &style_name, LV_PART_MAIN);
  lv_obj_add_style(person_organization, &style_organization_job, LV_PART_MAIN);
  lv_obj_add_style(person_job, &style_organization_job, LV_PART_MAIN);

  ui_person_set_secret(false);
  /* events */
  lv_obj_add_flag(screen_person, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(screen_person, ui_person_toggle_secret_event, LV_EVENT_LONG_PRESSED, NULL);
  lv_obj_add_event_cb(screen_person, ui_person_qrcode_event, LV_EVENT_DOUBLE_CLICKED, NULL);
  return (screen_person);
}

void ui_person_load() {
  lv_obj_add_flag(person_qr, LV_OBJ_FLAG_HIDDEN);
  ui_person_set_secret(false);
}
void ui_person_button_up() { ui_person_set_secret(true); }
void ui_person_button_down() { ui_person_set_secret(false); }