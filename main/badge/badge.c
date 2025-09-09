#include <stdio.h>
#include <string.h>
#include "cJSON.h"

#include "badge.h"

#include "led.h"
#include "bt.h"
#include "wifi/wifi.h"
#include "http/httpd.h"
#include "schedule.h"
#include "ui/ui.h"


badge_obj_t badge_obj;

 
char *load_file_content(const char *filename) {
  struct stat file_stat;
  if (stat(filename, &file_stat) == -1) {
    ESP_LOGI(__FILE__, "File not found");
    return NULL;
  }

  char *content_buf = (char *)calloc(1, file_stat.st_size + 1);
  if (!content_buf) {
    ESP_LOGI(__FILE__, "Cannot allocate memory");
  }
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    ESP_LOGI(__FILE__, "Cannot open file");
  }
  fread(content_buf, 1, file_stat.st_size, fp);
  fclose(fp);
  return content_buf;
}

cJSON *load_file(const char *filename) {
  ESP_LOGI(__FILE__, "Loading settings from %s", filename);
  char *default_content = load_file_content(filename);
  cJSON *json_content   = cJSON_Parse(default_content);
  free((char *)default_content);
  return json_content;
}

const char *json_get_str_value(cJSON *obj, const char *key) {
  ESP_LOGI(__FILE__, "Reading: %s", key);
  char *value = cJSON_GetObjectItem(obj, key)->valuestring;
  ESP_LOGI(__FILE__, "Get %s value: %s", key, value);
  return value;
}

int json_get_int_value(cJSON *obj, const char *key) {
  int value = cJSON_GetObjectItem(obj, key)->valueint;
  ESP_LOGI(__FILE__, "Get %s value: %d", key, value);
  return value;
}

void json_set_str_value(cJSON *obj, const char *key, const char *value) {
  if (cJSON_GetObjectItem(obj, key)) {
    cJSON_DeleteItemFromObject(obj, key);
  }

  cJSON_AddStringToObject(obj, key, value);
  ESP_LOGI(__FILE__, "Set %s value: %s", key, value);
}

void json_set_int_value(cJSON *obj, const char *key, const double value) {
  if (cJSON_GetObjectItem(obj, key)) {
    cJSON_DeleteItemFromObject(obj, key);
  }

  cJSON_AddNumberToObject(obj, key, value);
  ESP_LOGI(__FILE__, "Set %s value: %f", key, value);
}

void save_settings(cJSON *json_settings) {
  // Save to settings.json
  char *json = cJSON_Print(json_settings);
  FILE *fp   = fopen(SETTINGS_FILE, "w");
  fprintf(fp, "%s", json);
  cJSON_free(json);
  fclose(fp);

  ESP_LOGI(__FILE__, "Setting file saved");
  spiffs_size();
}

void save_badge_to_settings_file(cJSON *json_settings) {
  cJSON *obj_badge  = cJSON_GetObjectItem(json_settings, "badge");
  cJSON *obj_web    = cJSON_GetObjectItem(json_settings, "web");
  cJSON *obj_ap     = cJSON_GetObjectItem(json_settings, "ap");
  cJSON *obj_sync   = cJSON_GetObjectItem(json_settings, "sync");
  cJSON *obj_person = cJSON_GetObjectItem(json_settings, "person");
  cJSON *obj_secret = cJSON_GetObjectItem(json_settings, "secret");

  json_set_str_value(obj_badge, "name", badge_obj.device_name);
  json_set_str_value(obj_ap, "ssid", badge_obj.ap_ssid);
  json_set_str_value(obj_ap, "password", badge_obj.ap_password);
  json_set_str_value(obj_web, "login", badge_obj.web_login);
  json_set_str_value(obj_sync, "path", badge_obj.sync_path);

  json_set_str_value(obj_person, "name", badge_obj.person_name);
  json_set_str_value(obj_person, "organization", badge_obj.person_organization);
  json_set_str_value(obj_person, "job", badge_obj.person_job);
  json_set_str_value(obj_person, "message", badge_obj.person_message);

  json_set_str_value(obj_secret, "name", badge_obj.secret_name);
  json_set_str_value(obj_secret, "organization", badge_obj.secret_organization);
  json_set_str_value(obj_secret, "job", badge_obj.secret_job);
  json_set_str_value(obj_secret, "message", badge_obj.secret_message);

  save_settings(json_settings);
}

bool update_attribute(enum badge_item_id id, char *data) {
  switch (id) {
    case WEB_LOGIN_ID:  // Web login password
      snprintf(badge_obj.web_login, SIZEOF(badge_obj.web_login), "%s", data);
      break;
    case WIFI_SSID_ID:  // WiFi SSID
      snprintf(badge_obj.ap_ssid, SIZEOF(badge_obj.ap_ssid), "%s", data);
      break;
    case WIFI_PASSSWORD_ID:  // WiFi Password
      snprintf(badge_obj.ap_password, SIZEOF(badge_obj.ap_password), "%s", data);
      break;
    case DEVICE_NAME_ID:  // Device name
      snprintf(badge_obj.device_name, SIZEOF(badge_obj.device_name), "%s", data);
      break;
    case SYNC_PATH_ID:  // Sync path
      snprintf(badge_obj.sync_path, SIZEOF(badge_obj.sync_path), "%s", data);
      break;

    case PERSON_NAME_ID:  // person name
      snprintf(badge_obj.person_name, SIZEOF(badge_obj.person_name), "%s", data);
      break;
    case PERSON_ORGANIZATION_ID:  // person organization
      snprintf(badge_obj.person_organization, SIZEOF(badge_obj.person_organization), "%s", data);
      break;
    case PERSON_JOB_ID:  // person job
      snprintf(badge_obj.person_job, SIZEOF(badge_obj.person_job), "%s", data);
      break;
    case PERSON_MESSAGE_ID:  // person message
      snprintf(badge_obj.person_message, SIZEOF(badge_obj.person_message), "%s", data);
      break;
    case PERSON_URL_ID:  // person message
      snprintf(badge_obj.person_url, SIZEOF(badge_obj.person_url), "%s", data);
      break;

    case SECRET_NAME_ID:  // secret name
      snprintf(badge_obj.secret_name, SIZEOF(badge_obj.secret_name), "%s", data);
      break;
    case SECRET_ORGANIZATION_ID:  // secret organization
      snprintf(badge_obj.secret_organization, SIZEOF(badge_obj.secret_organization), "%s", data);
      break;
    case SECRET_JOB_ID:  // secret job
      snprintf(badge_obj.secret_job, SIZEOF(badge_obj.secret_job), "%s", data);
      break;
    case SECRET_MESSAGE_ID:  // secret message
      snprintf(badge_obj.secret_message, SIZEOF(badge_obj.secret_message), "%s", data);
      break;
    case SECRET_URL_ID:  // secret message
      snprintf(badge_obj.secret_url, SIZEOF(badge_obj.secret_url), "%s", data);
      break;
  }
  cJSON *json_settings = load_file(SETTINGS_FILE);
  save_badge_to_settings_file(json_settings);
  cJSON_Delete(json_settings);
  return true;
}

uint8_t generate_id(uint16_t seed) {
  // srand(seed);
  return 1 + (esp_random() % 7);
}

void badge_init() {
  // Init storage
  nvs_init();
  spiffs_init();

  // Init event loop
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  // Init settings
  struct stat file_stat;

  cJSON *json_settings = NULL;

  if (true || stat(SETTINGS_FILE, &file_stat) == -1) {
    json_settings = load_file(DEFAULT_FILE);

    cJSON *obj_badge   = cJSON_GetObjectItem(json_settings, "badge");
    cJSON *obj_web     = cJSON_GetObjectItem(json_settings, "web");
    cJSON *obj_ap      = cJSON_GetObjectItem(json_settings, "ap");
    cJSON *obj_sta     = cJSON_GetObjectItem(json_settings, "sta");
    cJSON *obj_sync    = cJSON_GetObjectItem(json_settings, "sync");
    cJSON *obj_person  = cJSON_GetObjectItem(json_settings, "person");
    cJSON *obj_secret  = cJSON_GetObjectItem(json_settings, "secret");
    cJSON *obj_display = cJSON_GetObjectItem(json_settings, "display");

    const char *web_login    = json_get_str_value(obj_web, "login");
    const char *sta_ssid     = json_get_str_value(obj_sta, "ssid");
    const char *sta_password = json_get_str_value(obj_sta, "password");
    const char *sync_path    = json_get_str_value(obj_sync, "path");

    const char *person_name         = json_get_str_value(obj_person, "name");
    const char *person_organization = json_get_str_value(obj_person, "organization");
    const char *person_job          = json_get_str_value(obj_person, "job");
    const char *person_message      = json_get_str_value(obj_person, "message");
    const char *person_url          = json_get_str_value(obj_person, "url");

    const char *secret_name         = json_get_str_value(obj_secret, "name");
    const char *secret_organization = json_get_str_value(obj_secret, "organization");
    const char *secret_job          = json_get_str_value(obj_secret, "job");
    const char *secret_message      = json_get_str_value(obj_secret, "message");
    const char *secret_url          = json_get_str_value(obj_secret, "url");

    // Load brightness settings with defaults if not present
    int brightness_max = obj_display ? json_get_int_value(obj_display, "brightness_max") : 255;
    int brightness_mid = obj_display ? json_get_int_value(obj_display, "brightness_mid") : 200;
    int brightness_off = obj_display ? json_get_int_value(obj_display, "brightness_off") : 0;

    // Update name and AP settings
    esp_efuse_mac_get_default(badge_obj.mac);
    badge_obj.short_mac = (badge_obj.mac[4] << 8) + badge_obj.mac[5];
    badge_obj.device_id = generate_id(badge_obj.short_mac);  // 1 + (badge_obj.short_mac % 7);
    
    snprintf(badge_obj.device_name, SIZEOF(badge_obj.device_name), "Saiyan-%04x", badge_obj.short_mac);
    snprintf(badge_obj.ap_ssid, SIZEOF(badge_obj.ap_ssid), "Saiyan-%04x", badge_obj.short_mac);
    snprintf(badge_obj.ap_password, SIZEOF(badge_obj.ap_password), "%02x%02x%02x%02x", badge_obj.mac[2], badge_obj.mac[3], badge_obj.mac[4], badge_obj.mac[5]);
    snprintf(badge_obj.web_login, SIZEOF(badge_obj.web_login), "%s", web_login);
    snprintf(badge_obj.sta_ssid, SIZEOF(badge_obj.sta_ssid), "%s", sta_ssid);
    snprintf(badge_obj.sta_password, SIZEOF(badge_obj.sta_password), "%s", sta_password);
    snprintf(badge_obj.sync_path, SIZEOF(badge_obj.sync_path), "%s", sync_path);

    snprintf(badge_obj.person_name, SIZEOF(badge_obj.person_name), "%s", person_name);
    snprintf(badge_obj.person_organization, SIZEOF(badge_obj.person_organization), "%s", person_organization);
    snprintf(badge_obj.person_job, SIZEOF(badge_obj.person_job), "%s", person_job);
    snprintf(badge_obj.person_message, SIZEOF(badge_obj.person_message), "%s", person_message);
    snprintf(badge_obj.person_url, SIZEOF(badge_obj.person_url), "%s", person_url);

    snprintf(badge_obj.secret_name, SIZEOF(badge_obj.secret_name), "%s", secret_name);
    snprintf(badge_obj.secret_organization, SIZEOF(badge_obj.secret_organization), "%s", secret_organization);
    snprintf(badge_obj.secret_job, SIZEOF(badge_obj.secret_job), "%s", secret_job);
    snprintf(badge_obj.secret_message, SIZEOF(badge_obj.secret_message), "%s", secret_message);
    snprintf(badge_obj.secret_url, SIZEOF(badge_obj.secret_url), "%s", secret_url);

    // Set brightness values
    badge_obj.brightness_max = (uint8_t)brightness_max;
    badge_obj.brightness_mid = (uint8_t)brightness_mid;
    badge_obj.brightness_off = (uint8_t)brightness_off;

    // Update settings object
    cJSON_AddNumberToObject(obj_badge, "id", (double)badge_obj.device_id);
    json_set_str_value(obj_badge, "name", badge_obj.device_name);
    json_set_str_value(obj_ap, "ssid", badge_obj.ap_ssid);
    json_set_str_value(obj_ap, "password", badge_obj.ap_password);
    json_set_str_value(obj_sync, "path", badge_obj.sync_path);

    json_set_str_value(obj_person, "name", badge_obj.person_name);
    json_set_str_value(obj_person, "organization", badge_obj.person_organization);
    json_set_str_value(obj_person, "job", badge_obj.person_job);
    json_set_str_value(obj_person, "message", badge_obj.person_message);
    json_set_str_value(obj_person, "url", badge_obj.person_url);

    json_set_str_value(obj_secret, "name", badge_obj.secret_name);
    json_set_str_value(obj_secret, "organization", badge_obj.secret_organization);
    json_set_str_value(obj_secret, "job", badge_obj.secret_job);
    json_set_str_value(obj_secret, "message", badge_obj.secret_message);
    json_set_str_value(obj_secret, "url", badge_obj.secret_url);

    // Save to settings.json
    save_settings(json_settings);

    ESP_LOGI(__FILE__, "Setting file saved to default values");
  }

  json_settings = load_file(SETTINGS_FILE);
  assert(json_settings != NULL);

  cJSON *obj_badge = cJSON_GetObjectItem(json_settings, "badge");
  cJSON *obj_web     = cJSON_GetObjectItem(json_settings, "web");
  cJSON *obj_ap      = cJSON_GetObjectItem(json_settings, "ap");
  cJSON *obj_sta     = cJSON_GetObjectItem(json_settings, "sta");
  cJSON *obj_sync    = cJSON_GetObjectItem(json_settings, "sync");
  cJSON *obj_person  = cJSON_GetObjectItem(json_settings, "person");
  cJSON *obj_secret  = cJSON_GetObjectItem(json_settings, "secret");
  cJSON *obj_display = cJSON_GetObjectItem(json_settings, "display");

  const char *badge_name   = json_get_str_value(obj_badge, "name");  // should be empty
  const char *web_login    = json_get_str_value(obj_web, "login");
  const char *ap_ssid      = json_get_str_value(obj_ap, "ssid");      // should be empty
  const char *ap_password  = json_get_str_value(obj_ap, "password");  // should be empty
  const char *sta_ssid     = json_get_str_value(obj_sta, "ssid");
  const char *sta_password = json_get_str_value(obj_sta, "password");
  const char *sync_path    = json_get_str_value(obj_sync, "path");

  const char *person_name         = json_get_str_value(obj_person, "name");
  const char *person_organization = json_get_str_value(obj_person, "organization");
  const char *person_job          = json_get_str_value(obj_person, "job");
  const char *person_message      = json_get_str_value(obj_person, "message");
  const char *person_url          = json_get_str_value(obj_person, "url");

  const char *secret_name         = json_get_str_value(obj_secret, "name");
  const char *secret_organization = json_get_str_value(obj_secret, "organization");
  const char *secret_job          = json_get_str_value(obj_secret, "job");
  const char *secret_message      = json_get_str_value(obj_secret, "message");
  const char *secret_url          = json_get_str_value(obj_secret, "url");

  // Load brightness settings with defaults if not present
  int brightness_max = obj_display ? json_get_int_value(obj_display, "brightness_max") : 255;
  int brightness_mid = obj_display ? json_get_int_value(obj_display, "brightness_mid") : 200;
  int brightness_off = obj_display ? json_get_int_value(obj_display, "brightness_off") : 0;

  // Update badge object
  esp_efuse_mac_get_default(badge_obj.mac);
  badge_obj.short_mac = (badge_obj.mac[4] << 8) + badge_obj.mac[5];
  badge_obj.device_id = (int)cJSON_GetObjectItem(obj_badge, "id")->valuedouble;
  // generate_id(badge_obj.short_mac);//1 + (badge_obj.short_mac % 7);

  snprintf(badge_obj.device_name, SIZEOF(badge_obj.device_name), "%s", badge_name);
  snprintf(badge_obj.web_login, SIZEOF(badge_obj.web_login), "%s", web_login);
  snprintf(badge_obj.ap_ssid, SIZEOF(badge_obj.ap_ssid), "%s", ap_ssid);
  snprintf(badge_obj.ap_password, SIZEOF(badge_obj.ap_password), "%s", ap_password);
  snprintf(badge_obj.sta_ssid, SIZEOF(badge_obj.sta_ssid), "%s", sta_ssid);
  snprintf(badge_obj.sta_password, SIZEOF(badge_obj.sta_password), "%s", sta_password);
  snprintf(badge_obj.sync_path, SIZEOF(badge_obj.sync_path), "%s", sync_path);

  snprintf(badge_obj.person_name, SIZEOF(badge_obj.person_name), "%s", person_name);
  snprintf(badge_obj.person_organization, SIZEOF(badge_obj.person_organization), "%s", person_organization);
  snprintf(badge_obj.person_job, SIZEOF(badge_obj.person_job), "%s", person_job);
  snprintf(badge_obj.person_message, SIZEOF(badge_obj.person_message), "%s", person_message);
  snprintf(badge_obj.person_url, SIZEOF(badge_obj.person_url), "%s", person_url);

  snprintf(badge_obj.secret_name, SIZEOF(badge_obj.secret_name), "%s", secret_name);
  snprintf(badge_obj.secret_organization, SIZEOF(badge_obj.secret_organization), "%s", secret_organization);
  snprintf(badge_obj.secret_job, SIZEOF(badge_obj.secret_job), "%s", secret_job);
  snprintf(badge_obj.secret_message, SIZEOF(badge_obj.secret_message), "%s", secret_message);
  snprintf(badge_obj.secret_url, SIZEOF(badge_obj.secret_url), "%s", secret_url);
  // Set brightness values
  badge_obj.brightness_max = (uint8_t)brightness_max;
  badge_obj.brightness_mid = (uint8_t)brightness_mid;
  badge_obj.brightness_off = (uint8_t)brightness_off;

  ESP_LOGI(__FILE__, "Setting file loaded");
  ESP_LOGI(__FILE__, "The badge ID is: %d", badge_obj.device_id);
  cJSON_Delete(json_settings);
  badge_obj.update = update_attribute;
}
