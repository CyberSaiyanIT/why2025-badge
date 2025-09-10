#include "admin.h"
#include "../wifi/wifi.h"
#include "../wifi/info.h"

#define ADMIN_INFO_SIZE 300
static lv_obj_t *admin_switch_ap, *admin_switch_sta;
static lv_obj_t *admin_switch_ap_text, *admin_switch_sta_text;
static lv_obj_t *admin_info;

static uint8_t admin_state = ADMIN_STATE_OFF;

void ui_update_admin_info() {
  wifi_info_t wifi_info;
  char admin_info_text[ADMIN_INFO_SIZE];

  if (update_ip_info(&wifi_info) != ESP_OK) {
    lv_obj_add_flag(admin_info, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  if (wifi_info.wifi_mode == WIFI_MODE_AP)
    snprintf(admin_info_text, ADMIN_INFO_SIZE, "SSID: %s\nPassword: %s\nIP: %s\nGateway: %s\nNetmask: %s\n\nConnect to http://%s",
             badge_obj.ap_ssid, badge_obj.ap_password,
             wifi_info.ip, wifi_info.gateway, wifi_info.netmask,
             wifi_info.gateway);
  else if (wifi_info.wifi_mode == WIFI_MODE_STA)
    snprintf(admin_info_text, ADMIN_INFO_SIZE, "SSID: %s\n\nIP: %s\nGateway: %s\nNetmask: %s\n\nConnect to http://%s",
             badge_obj.sta_ssid,
             wifi_info.ip, wifi_info.gateway, wifi_info.netmask,
             wifi_info.gateway);
  else {
    lv_obj_add_flag(admin_info, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_label_set_text(admin_info, admin_info_text);
  lv_obj_clear_flag(admin_info, LV_OBJ_FLAG_HIDDEN);
}

void ui_ap_start_handler() {
  ESP_LOGI("UI", "AP started handler called");
  lv_label_set_text(admin_switch_ap_text, "TURN OFF AP");

  // Update IP information immediately
  ui_update_admin_info();

  lv_obj_add_state(admin_switch_ap, LV_STATE_PRESSED | LV_STATE_CHECKED);
  admin_state = ADMIN_STATE_AP;
}

void ui_ap_stop_handler() {
  lv_label_set_text(admin_switch_ap_text, "TURN ON AP");
  lv_obj_add_flag(admin_info, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(admin_switch_sta, LV_OBJ_FLAG_HIDDEN);

  lv_obj_clear_state(admin_switch_ap, LV_STATE_PRESSED | LV_STATE_CHECKED);
  admin_state = ADMIN_STATE_OFF;
}

void ui_sta_connected_handler() {
  ESP_LOGI("UI", "STA connected handler called");
  ESP_LOGI("UI", "Current admin_state: %d", admin_state);

  lv_obj_add_state(admin_switch_sta, LV_STATE_PRESSED | LV_STATE_CHECKED);
  lv_label_set_text(admin_switch_sta_text, "Downloading...");

  // Update IP information when connected as station immediately
  ESP_LOGI("UI", "About to call update_ip_info from STA connected handler");
  ui_update_admin_info();
  ESP_LOGI("UI", "update_ip_info call completed from STA connected handler");

  admin_state = ADMIN_STATE_STA;
}

void ui_sta_disconnected_handler() {
  lv_obj_clear_state(admin_switch_sta, LV_STATE_PRESSED | LV_STATE_CHECKED);
  lv_obj_add_flag(admin_info, LV_OBJ_FLAG_HIDDEN);
  admin_state = ADMIN_STATE_OFF;
}

void ui_sta_stop_handler() {
  lv_label_set_text(admin_switch_sta_text, "SYNC SCHEDULE");
  lv_obj_add_flag(admin_info, LV_OBJ_FLAG_HIDDEN);
  admin_state = ADMIN_STATE_OFF;
}

void ui_manual_ip_update() {
  ESP_LOGI("UI", "Manual IP update triggered from admin screen");
  ui_update_admin_info();
}

void ui_force_show_ip_labels() {
  ESP_LOGI("UI", "Force showing IP labels for testing - calling update_ip_info instead");
  ui_update_admin_info();
}

void ui_connection_progress(uint8_t cur, uint8_t max) {
  if (cur != max) {
    char buf[BADGE_BUF_SIZE + 20] = {0};  // Increase the size of buf to accommodate the entire formatted
                                          // string
    snprintf(buf, sizeof(buf), "Connecting (%d/%d)", cur, max);
    lv_label_set_text(admin_switch_sta_text, buf);
    lv_obj_clear_flag(admin_switch_sta_text, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text(admin_switch_sta_text, "Connection failed!");
    lv_obj_clear_flag(admin_switch_sta_text, LV_OBJ_FLAG_HIDDEN);
  }
}

static void ui_screen_admin_switch(lv_event_t *event) { admin_button_up(); }

static void ui_screen_admin_switch_sta(lv_event_t *event) { admin_button_down(); }

lv_obj_t *ui_screen_admin_init() {
  // page for admin
  lv_obj_t *screen_admin = lv_obj_create(NULL);
  /* ADMIN SWITCH */
  admin_switch_ap = lv_button_create(screen_admin);
  lv_obj_set_width(admin_switch_ap, 250);
  lv_obj_align(admin_switch_ap, LV_ALIGN_TOP_MID, 0, 10);
  lv_obj_add_event_cb(admin_switch_ap, ui_screen_admin_switch, LV_EVENT_SINGLE_CLICKED, NULL);
  admin_switch_ap_text = lv_label_create(admin_switch_ap);
  lv_label_set_text(admin_switch_ap_text, "TURN ON AP");
  lv_obj_center(admin_switch_ap_text);

  admin_switch_sta = lv_button_create(screen_admin);
  lv_obj_set_width(admin_switch_sta, 250);
  lv_obj_align(admin_switch_sta, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_add_event_cb(admin_switch_sta, ui_screen_admin_switch_sta, LV_EVENT_SINGLE_CLICKED, NULL);
  admin_switch_sta_text = lv_label_create(admin_switch_sta);
  lv_label_set_text(admin_switch_sta_text, "SYNC SCHEDULE");
  lv_obj_center(admin_switch_sta_text);

  admin_info = lv_label_create(screen_admin);
  lv_obj_align(admin_info, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_flag(admin_info, LV_OBJ_FLAG_HIDDEN);

  return (screen_admin);
}

void admin_button_up() {
  switch (admin_state) {
    case ADMIN_STATE_OFF:  // AP and STA disabled: enable AP
      start_ap();
      lv_obj_add_flag(admin_switch_sta, LV_OBJ_FLAG_HIDDEN);
      admin_state = ADMIN_STATE_AP;
      break;
    case ADMIN_STATE_AP:  // AP enabled: disable AP
      stop_all();
      lv_obj_clear_flag(admin_switch_sta, LV_OBJ_FLAG_HIDDEN);
      admin_state = ADMIN_STATE_OFF;
      break;
    case ADMIN_STATE_STA:  // STA connected: manual IP refresh
      ESP_LOGI("UI", "Manual IP refresh triggered via UP button");
      ui_manual_ip_update();
      // Also test forcing labels to be visible for debugging
      ui_force_show_ip_labels();
      break;
  }
}

void admin_button_down() {
  switch (admin_state) {
    case ADMIN_STATE_OFF:  // AP and STA disabled: enable STA
      start_sta();
      lv_label_set_text(admin_switch_sta_text, "Started...");
      admin_state = ADMIN_STATE_STA;
      break;
    case ADMIN_STATE_AP:  // AP enabled: test showing IP labels
      ESP_LOGI("UI", "Force show IP labels test (DOWN button in AP mode)");
      ui_force_show_ip_labels();
      break;
    case ADMIN_STATE_STA:  // STA mode: test showing IP labels
      ESP_LOGI("UI", "Force show IP labels test (DOWN button in STA mode)");
      ui_force_show_ip_labels();
      break;
  }
}