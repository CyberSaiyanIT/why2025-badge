#include "admin.h"

static lv_obj_t *admin_switch, *admin_switch_sta, *admin_sync;
static lv_obj_t *admin_switch_text, *admin_switch_sta_text, *admin_sync_text;
static lv_obj_t *admin_ssid, *admin_pass;
static lv_obj_t *admin_client_ip, *admin_gateway_ip;

static bool ap_started = false;
static bool sta_connected = false;

static uint8_t admin_state = ADMIN_STATE_OFF;

void ui_send_wifi_event(int event) {
  xQueueSend(wifi_queue, &event, portMAX_DELAY);
}

void ui_ap_start_handler() {
  ap_started = true;

  ESP_LOGI("UI", "AP started handler called");
  lv_label_set_text(admin_switch_text, "TURN OFF AP");

  char buf[BADGE_BUF_SIZE + 19] = {0};
  snprintf(buf, sizeof(buf), "SSID: %s", badge_obj.ap_ssid);
  lv_label_set_text(admin_ssid, buf);
  snprintf(buf, sizeof(buf), "PASS: %s", badge_obj.ap_password);
  lv_label_set_text(admin_pass, buf);

  lv_obj_clear_flag(admin_ssid, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(admin_pass, LV_OBJ_FLAG_HIDDEN);

  // Update IP information immediately
  ui_update_ip_info();

  // TODO: Also create a delayed task to retry getting IP info
  // xTaskCreate(ui_delayed_ip_update_task, "delayed_ip_update", 2048, NULL, 5,
  // NULL);

  lv_obj_add_state(admin_switch, LV_STATE_PRESSED | LV_STATE_CHECKED);
  admin_state = ADMIN_STATE_AP;
}

void ui_ap_stop_handler() {
  ap_started = false;

  lv_label_set_text(admin_switch_text, "TURN ON AP");
  lv_obj_add_flag(admin_ssid, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(admin_pass, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(admin_client_ip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(admin_gateway_ip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(admin_switch_sta, LV_OBJ_FLAG_HIDDEN);

  lv_obj_clear_state(admin_switch, LV_STATE_PRESSED | LV_STATE_CHECKED);
  // lv_btn_set_state(admin_switch,
  // LV_BTN_STATE_RELEASED);//enabl(admin_switch);
  admin_state = ADMIN_STATE_OFF;
}

void ui_sta_connected_handler() {
  sta_connected = true;

  ESP_LOGI("UI", "STA connected handler called");
  ESP_LOGI("UI", "Current admin_state: %d", admin_state);
  // ESP_LOGI("UI", "Current screen: %d", current_screen);

  lv_obj_add_state(admin_switch_sta, LV_STATE_PRESSED | LV_STATE_CHECKED);
  // lv_btn_set_state(admin_switch_sta, LV_BTN_STATE_CHECKED_PRESSED);
  lv_label_set_text(admin_switch_sta_text, "Downloading...");

  // Update IP information when connected as station immediately
  ESP_LOGI("UI", "About to call ui_update_ip_info from STA connected handler");
  ui_update_ip_info();
  ESP_LOGI("UI", "ui_update_ip_info call completed from STA connected handler");

  // TODO: Also create a delayed task to retry getting IP info
  // xTaskCreate(ui_delayed_ip_update_task, "delayed_ip_update", 2048, NULL, 5,
  // NULL);

  admin_state = ADMIN_STATE_STA;
}

void ui_sta_disconnected_handler() {
  sta_connected = false;
  lv_obj_clear_state(admin_switch_sta, LV_STATE_PRESSED | LV_STATE_CHECKED);
  // lv_btn_set_state(admin_switch_sta, LV_BTN_STATE_RELEASED);
  lv_obj_add_flag(admin_client_ip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(admin_gateway_ip, LV_OBJ_FLAG_HIDDEN);
  admin_state = ADMIN_STATE_OFF;
}

void ui_sta_stop_handler() {
  sta_connected = false;
  lv_label_set_text(admin_switch_sta_text, "SYNC SCHEDULE");
  lv_obj_add_flag(admin_client_ip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(admin_gateway_ip, LV_OBJ_FLAG_HIDDEN);
  admin_state = ADMIN_STATE_OFF;
}

void ui_manual_ip_update() {
  ESP_LOGI("UI", "Manual IP update triggered from admin screen");
  ui_update_ip_info();
}

void ui_force_show_ip_labels() {
  ESP_LOGI("UI", "Force showing IP labels for testing - calling "
                 "ui_update_ip_info instead");
  ui_update_ip_info();
}

void ui_connection_progress(uint8_t cur, uint8_t max) {
  if (cur != max) {
    char buf[BADGE_BUF_SIZE + 20] = {
        0}; // Increase the size of buf to accommodate the entire formatted
            // string
    snprintf(buf, sizeof(buf), "Connecting (%d/%d)", cur, max);
    lv_label_set_text(admin_switch_sta_text, buf);
    lv_obj_clear_flag(admin_switch_sta_text, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text(admin_switch_sta_text, "Connection failed!");
    lv_obj_clear_flag(admin_switch_sta_text, LV_OBJ_FLAG_HIDDEN);
  }
}

void ui_toggle_sync() {
  lv_obj_clear_state(admin_sync, LV_STATE_PRESSED | LV_STATE_CHECKED);
  // lv_btn_set_state(admin_sync, LV_BTN_STATE_RELEASED);
  ui_send_wifi_event(EVENT_STA_STOP);
}

void ui_list_all_netifs() {
  ESP_LOGI("UI", "=== LISTING ALL NETWORK INTERFACES ===");

  // Try to iterate through all available network interfaces
  esp_netif_t *netif = NULL;
  esp_netif_t *temp_netif = esp_netif_next_unsafe(netif);
  int count = 0;

  while (temp_netif != NULL) {
    count++;
    ESP_LOGI("UI", "Found netif %d: %p", count, temp_netif);

    // Get interface description
    const char *desc = esp_netif_get_desc(temp_netif);
    ESP_LOGI("UI", "Interface %d description: %s", count,
             desc ? desc : "unknown");

    // Get IP info for this interface
    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(temp_netif, &ip_info);
    ESP_LOGI("UI", "Interface %d IP info (ret: %s):", count,
             esp_err_to_name(ret));
    ESP_LOGI("UI", "  IP: " IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI("UI", "  Gateway: " IPSTR, IP2STR(&ip_info.gw));
    ESP_LOGI("UI", "  Netmask: " IPSTR, IP2STR(&ip_info.netmask));

    temp_netif = esp_netif_next_unsafe(temp_netif);
  }

  ESP_LOGI("UI", "Total network interfaces found: %d", count);
  ESP_LOGI("UI", "=== END NETIF LISTING ===");
}

void ui_update_ip_info() {
  char buf[BADGE_BUF_SIZE + 30] = {0};

  ESP_LOGI("UI", "=== IP INFO DEBUG ===");
  ESP_LOGI("UI", "sta_connected: %s, ap_started: %s, admin_state: %d",
           sta_connected ? "true" : "false", ap_started ? "true" : "false",
           admin_state);

  // First, list all network interfaces for debugging
  ui_list_all_netifs();

  // Get AP interface and show AP IP when AP is started
  if (ap_started) {
    esp_netif_t *ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    ESP_LOGI("UI", "AP netif handle (WIFI_AP_DEF): %p", ap_netif);

    if (ap_netif) {
      esp_netif_ip_info_t ip_info;
      esp_err_t ret = esp_netif_get_ip_info(ap_netif, &ip_info);
      ESP_LOGI("UI", "AP esp_netif_get_ip_info returned: %s",
               esp_err_to_name(ret));
      ESP_LOGI("UI", "AP IP: " IPSTR, IP2STR(&ip_info.ip));
      ESP_LOGI("UI", "AP Gateway: " IPSTR, IP2STR(&ip_info.gw));
      ESP_LOGI("UI", "AP Netmask: " IPSTR, IP2STR(&ip_info.netmask));

      if (ret == ESP_OK && ip_info.ip.addr != 0) {
        // Show AP IP information
        snprintf(buf, sizeof(buf), "AP IP: " IPSTR, IP2STR(&ip_info.ip));
        lv_label_set_text(admin_client_ip, buf);
        lv_obj_clear_flag(admin_client_ip, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI("UI", "Set admin_client_ip text to: %s", buf);

        snprintf(buf, sizeof(buf), "\nConnect to\nhttp://" IPSTR,
                 IP2STR(&ip_info.gw));
        lv_label_set_text(admin_gateway_ip, buf);
        lv_obj_clear_flag(admin_gateway_ip, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI("UI", "Set admin_gateway_ip text to: %s", buf);
        ESP_LOGI("UI", "Successfully displayed AP IP info");
        return;
      }
    }
  }

  // Get STA interface and show STA IP when connected as station
  if (sta_connected) {
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    ESP_LOGI("UI", "STA netif handle (WIFI_STA_DEF): %p", sta_netif);

    if (sta_netif) {
      esp_netif_ip_info_t ip_info;
      esp_err_t ret = esp_netif_get_ip_info(sta_netif, &ip_info);
      ESP_LOGI("UI", "STA esp_netif_get_ip_info returned: %s",
               esp_err_to_name(ret));
      ESP_LOGI("UI", "STA IP: " IPSTR, IP2STR(&ip_info.ip));
      ESP_LOGI("UI", "STA Gateway: " IPSTR, IP2STR(&ip_info.gw));
      ESP_LOGI("UI", "STA Netmask: " IPSTR, IP2STR(&ip_info.netmask));

      if (ret == ESP_OK && ip_info.ip.addr != 0) {
        ESP_LOGI("UI", "STA IP is valid, updating UI labels...");
        snprintf(buf, sizeof(buf), "Client IP: " IPSTR, IP2STR(&ip_info.ip));
        lv_label_set_text(admin_client_ip, buf);
        lv_obj_clear_flag(admin_client_ip, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI("UI", "Set admin_client_ip text to: %s", buf);

        snprintf(buf, sizeof(buf), "Gateway: " IPSTR, IP2STR(&ip_info.gw));
        lv_label_set_text(admin_gateway_ip, buf);
        lv_obj_clear_flag(admin_gateway_ip, LV_OBJ_FLAG_HIDDEN);
        ESP_LOGI("UI", "Set admin_gateway_ip text to: %s", buf);
        ESP_LOGI("UI", "Successfully displayed STA IP info");
        return;
      } else {
        ESP_LOGW(
            "UI",
            "STA IP is not valid or error occurred. ret=%s, ip.addr=0x%08x",
            esp_err_to_name(ret), ip_info.ip.addr);
      }
    } else {
      ESP_LOGW("UI", "Could not get STA netif handle");
    }
  }

  // If we reach here, we couldn't get IP info through normal methods
  // Try iterating through all interfaces as fallback
  ESP_LOGI(
      "UI",
      "Primary methods failed, trying to iterate through all interfaces...");

  esp_netif_t *netif = NULL;
  esp_netif_t *temp_netif = esp_netif_next_unsafe(netif);
  bool ip_found = false;

  while (temp_netif != NULL && !ip_found) {
    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(temp_netif, &ip_info);

    if (ret == ESP_OK && ip_info.ip.addr != 0) {
      const char *desc = esp_netif_get_desc(temp_netif);
      ESP_LOGI("UI", "Found valid IP on interface %s: " IPSTR,
               desc ? desc : "unknown", IP2STR(&ip_info.ip));

      // Use interface description to determine type instead of IP range
      // heuristic
      if (desc && strstr(desc, "ap")) {
        // AP interface
        snprintf(buf, sizeof(buf), "AP IP: " IPSTR, IP2STR(&ip_info.ip));
        lv_label_set_text(admin_client_ip, buf);
        lv_obj_clear_flag(admin_client_ip, LV_OBJ_FLAG_HIDDEN);

        snprintf(buf, sizeof(buf), "\nConnect to\nhttp://" IPSTR,
                 IP2STR(&ip_info.gw));
        lv_label_set_text(admin_gateway_ip, buf);
        lv_obj_clear_flag(admin_gateway_ip, LV_OBJ_FLAG_HIDDEN);
        ip_found = true;
      } else if (desc && strstr(desc, "sta")) {
        // STA interface
        snprintf(buf, sizeof(buf), "Client IP: " IPSTR, IP2STR(&ip_info.ip));
        lv_label_set_text(admin_client_ip, buf);
        lv_obj_clear_flag(admin_client_ip, LV_OBJ_FLAG_HIDDEN);

        snprintf(buf, sizeof(buf), "\nConnect to\nhttp://" IPSTR,
                 IP2STR(&ip_info.gw));
        lv_label_set_text(admin_gateway_ip, buf);
        lv_obj_clear_flag(admin_gateway_ip, LV_OBJ_FLAG_HIDDEN);
        ip_found = true;
      }
      ESP_LOGI("UI", "Successfully displayed IP info from interface iteration");
    }

    temp_netif = esp_netif_next_unsafe(temp_netif);
  }

  if (!ip_found) {
    ESP_LOGW("UI", "No valid IP information found to display");
    lv_obj_add_flag(admin_client_ip, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(admin_gateway_ip, LV_OBJ_FLAG_HIDDEN);
  }

  ESP_LOGI("UI", "=== END IP INFO DEBUG ===");
}

lv_obj_t *ui_screen_admin_init() {
  // page for admin
  lv_obj_t *screen_admin = lv_obj_create(NULL);
  /* ADMIN SWITCH */
  admin_switch = lv_btn_create(screen_admin);
  lv_obj_set_size(admin_switch, 200, 50);
  lv_obj_set_pos(admin_switch, 60, 35);
  admin_switch_text = lv_label_create(admin_switch);
  lv_label_set_text(admin_switch_text, "TURN ON AP");

  admin_sync = lv_btn_create(screen_admin);
  lv_obj_set_size(admin_sync, 200, 50);
  lv_obj_set_pos(admin_sync, 60, 180);
  admin_sync_text = lv_label_create(admin_sync);
  lv_label_set_text(admin_sync_text, "FORCE SCHEDULE SYNC");
  lv_obj_add_flag(admin_sync, LV_OBJ_FLAG_HIDDEN);

  admin_ssid = lv_label_create(screen_admin);
  lv_obj_align_to(admin_ssid, admin_switch, LV_ALIGN_OUT_BOTTOM_MID, -50, 10);
  lv_obj_add_flag(admin_ssid, LV_OBJ_FLAG_HIDDEN);
  admin_pass = lv_label_create(screen_admin);
  lv_obj_align_to(admin_pass, admin_switch, LV_ALIGN_OUT_BOTTOM_MID, -50, 30);
  lv_obj_add_flag(admin_pass, LV_OBJ_FLAG_HIDDEN);

  admin_client_ip = lv_label_create(screen_admin);
  lv_obj_align_to(admin_client_ip, admin_switch, LV_ALIGN_OUT_BOTTOM_MID, -50,
                  50);
  lv_label_set_text(admin_client_ip, "Client IP: [Not Connected]");
  lv_obj_add_flag(admin_client_ip, LV_OBJ_FLAG_HIDDEN);
  admin_gateway_ip = lv_label_create(screen_admin);
  lv_obj_align_to(admin_gateway_ip, admin_switch, LV_ALIGN_OUT_BOTTOM_MID, -50,
                  70);
  lv_label_set_text(admin_gateway_ip, "Gateway: [Not Available]");
  lv_obj_add_flag(admin_gateway_ip, LV_OBJ_FLAG_HIDDEN);

  admin_switch_sta = lv_btn_create(screen_admin);
  lv_obj_set_size(admin_switch_sta, 200, 50);
  lv_obj_set_pos(admin_switch_sta, 60, 180);
  admin_switch_sta_text = lv_label_create(admin_switch_sta);
  lv_label_set_text(admin_switch_sta_text, "SYNC SCHEDULE");

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_START,
                                             &ui_ap_start_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STOP,
                                             &ui_ap_stop_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &ui_sta_connected_handler, NULL));
  ESP_ERROR_CHECK(
      esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                 &ui_sta_disconnected_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_STOP,
                                             &ui_sta_stop_handler, NULL));

  return (screen_admin);
}

void admin_button_up() {
  switch (admin_state) {
  case ADMIN_STATE_OFF: // AP and STA disabled: enable AP
    ui_send_wifi_event(EVENT_HOTSPOT_START);
    lv_obj_add_flag(admin_switch_sta, LV_OBJ_FLAG_HIDDEN);
    admin_state = ADMIN_STATE_AP;
    break;
  case ADMIN_STATE_AP: // AP enabled: disable AP
    ui_send_wifi_event(EVENT_HOTSPOT_STOP);
    lv_obj_clear_flag(admin_switch_sta, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(admin_ssid, LV_OBJ_FLAG_HIDDEN);
    admin_state = ADMIN_STATE_OFF;
    break;
  case ADMIN_STATE_STA: // STA connected: manual IP refresh
    ESP_LOGI("UI", "Manual IP refresh triggered via UP button");
    ui_manual_ip_update();
    // Also test forcing labels to be visible for debugging
    ui_force_show_ip_labels();
    break;
  }
}

void admin_button_down() {
  switch (admin_state) {
  case ADMIN_STATE_OFF: // AP and STA disabled: enable STA
    ui_send_wifi_event(EVENT_STA_START);
    lv_label_set_text(admin_switch_sta_text, "Started...");
    admin_state = ADMIN_STATE_STA;
    break;
  case ADMIN_STATE_AP: // AP enabled: test showing IP labels
    ESP_LOGI("UI", "Force show IP labels test (DOWN button in AP mode)");
    ui_force_show_ip_labels();
    break;
  case ADMIN_STATE_STA: // STA mode: test showing IP labels
    ESP_LOGI("UI", "Force show IP labels test (DOWN button in STA mode)");
    ui_force_show_ip_labels();
    break;
  }
}