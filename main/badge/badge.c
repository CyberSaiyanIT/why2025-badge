#include "badge.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "cJSON.h"

#include "esp_random.h"
#include "common/storage.h"

badge_obj_t badge_obj;

static cJSON *badge_load_file(const char *filename) {
  struct stat file_stat;

  ESP_LOGI(__FILE__, "Loading settings from %s", filename);
  if (stat(filename, &file_stat) == -1) {
    ESP_LOGE(__FILE__, "File not found");
    return NULL;
  }

  FILE *fp = fopen(filename, "r");
  if (!fp) {
    ESP_LOGE(__FILE__, "Cannot open file");
    return NULL;
  }

  char *content_buf = (char *)calloc(1, file_stat.st_size + 1);
  if (!content_buf) {
    ESP_LOGE(__FILE__, "Cannot allocate memory");
    return NULL;
  }

  fread(content_buf, 1, file_stat.st_size, fp);
  fclose(fp);

  cJSON *json_content = cJSON_Parse(content_buf);
  free(content_buf);
  assert(json_content != NULL);
  return json_content;
}

static esp_err_t badge_copy_default_to_settings() {
  FILE *default_file;
  FILE *settings_file;
  int c = 0;

  ESP_LOGI(__FILE__, "Copy %s to %s", DEFAULT_FILE, SETTINGS_FILE);
  // Open default file for reading
  if ((default_file = fopen(DEFAULT_FILE, "r")) == NULL) {
    ESP_LOGE(__FILE__, "Cannot open file %s", DEFAULT_FILE);
    return ESP_FAIL;
  }

  // Open settings file for writing
  if ((settings_file = fopen(SETTINGS_FILE, "w")) == NULL) {
    fclose(default_file);
    ESP_LOGE(__FILE__, "Cannot open file %s", SETTINGS_FILE);
    return ESP_FAIL;
  }

  // Read contents from file
  while ((c = fgetc(default_file)) != EOF)
    fputc(c, settings_file);

  fclose(default_file);
  fclose(settings_file);
  return ESP_OK;
}

static const char *json_get_str_value(cJSON *obj, const char *key) {
  char *value = cJSON_GetObjectItem(obj, key)->valuestring;
  ESP_LOGI(__FILE__, "Get %s value: %s", key, value);
  return value;
}

static int json_get_int_value(cJSON *obj, const char *key) {
  int value = cJSON_GetObjectItem(obj, key)->valueint;
  ESP_LOGI(__FILE__, "Get %s value: %d", key, value);
  return value;
}

static void json_set_str_value(cJSON *obj, const char *key, const char *value) {
  if (cJSON_GetObjectItem(obj, key))
    cJSON_DeleteItemFromObject(obj, key);

  cJSON_AddStringToObject(obj, key, value);
  ESP_LOGI(__FILE__, "Set %s value: %s", key, value);
}

static void badge_save_settings(cJSON *json_settings) {
  // Save to settings.json
  char *json = cJSON_Print(json_settings);
  FILE *fp   = fopen(SETTINGS_FILE, "w");
  fprintf(fp, "%s", json);
  cJSON_free(json);
  fclose(fp);

  ESP_LOGI(__FILE__, "Setting file saved");
  spiffs_size();
}

static void badge_save_to_settings_file(cJSON *json_settings) {
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

  json_set_str_value(obj_person, "name", badge_obj.person.name);
  json_set_str_value(obj_person, "organization", badge_obj.person.organization);
  json_set_str_value(obj_person, "job", badge_obj.person.job);
  json_set_str_value(obj_person, "message", badge_obj.person.message);

  json_set_str_value(obj_secret, "name", badge_obj.secret.name);
  json_set_str_value(obj_secret, "organization", badge_obj.secret.organization);
  json_set_str_value(obj_secret, "job", badge_obj.secret.job);
  json_set_str_value(obj_secret, "message", badge_obj.secret.message);

  badge_save_settings(json_settings);
}

bool badge_update_attribute(enum badge_item_id id, char *data) {
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
      snprintf(badge_obj.person.name, SIZEOF(badge_obj.person.name), "%s", data);
      break;
    case PERSON_ORGANIZATION_ID:  // person organization
      snprintf(badge_obj.person.organization, SIZEOF(badge_obj.person.organization), "%s", data);
      break;
    case PERSON_JOB_ID:  // person job
      snprintf(badge_obj.person.job, SIZEOF(badge_obj.person.job), "%s", data);
      break;
    case PERSON_MESSAGE_ID:  // person message
      snprintf(badge_obj.person.message, SIZEOF(badge_obj.person.message), "%s", data);
      break;
    case PERSON_URL_ID:  // person message
      snprintf(badge_obj.person.url, SIZEOF(badge_obj.person.url), "%s", data);
      break;

    case SECRET_NAME_ID:  // secret name
      snprintf(badge_obj.secret.name, SIZEOF(badge_obj.secret.name), "%s", data);
      break;
    case SECRET_ORGANIZATION_ID:  // secret organization
      snprintf(badge_obj.secret.organization, SIZEOF(badge_obj.secret.organization), "%s", data);
      break;
    case SECRET_JOB_ID:  // secret job
      snprintf(badge_obj.secret.job, SIZEOF(badge_obj.secret.job), "%s", data);
      break;
    case SECRET_MESSAGE_ID:  // secret message
      snprintf(badge_obj.secret.message, SIZEOF(badge_obj.secret.message), "%s", data);
      break;
    case SECRET_URL_ID:  // secret message
      snprintf(badge_obj.secret.url, SIZEOF(badge_obj.secret.url), "%s", data);
      break;
    default:
      ESP_LOGE(__FILE__, "Attribute not found: %d", id);
      return false;
      break;
  }
  cJSON *json_settings = badge_load_file(SETTINGS_FILE);
  badge_save_to_settings_file(json_settings);
  cJSON_Delete(json_settings);
  return true;
}

static uint8_t badge_generate_id() { return 1 + (esp_random() % 7); }

static void badge_get_person_from_json(cJSON *json_person, person_t *person) {
  const char *name         = json_get_str_value(json_person, "name");
  const char *organization = json_get_str_value(json_person, "organization");
  const char *job          = json_get_str_value(json_person, "job");
  const char *message      = json_get_str_value(json_person, "message");
  const char *url          = json_get_str_value(json_person, "url");

  snprintf(person->name, SIZEOF(person->name), "%s", name);
  snprintf(person->organization, SIZEOF(person->organization), "%s", organization);
  snprintf(person->job, SIZEOF(person->job), "%s", job);
  snprintf(person->message, SIZEOF(person->message), "%s", message);
  snprintf(person->url, SIZEOF(person->url), "%s", url);
}

void badge_init() {
  // Init settings
  struct stat file_stat;
  bool default_loaded = false;

  if (stat(SETTINGS_FILE, &file_stat) == -1) {
    ESP_ERROR_CHECK(badge_copy_default_to_settings());
    default_loaded = true;
  }

  cJSON *json_settings = badge_load_file(SETTINGS_FILE);
  cJSON *obj_badge     = cJSON_GetObjectItem(json_settings, "badge");
  cJSON *obj_web       = cJSON_GetObjectItem(json_settings, "web");
  cJSON *obj_ap        = cJSON_GetObjectItem(json_settings, "ap");
  cJSON *obj_sta       = cJSON_GetObjectItem(json_settings, "sta");
  cJSON *obj_sync      = cJSON_GetObjectItem(json_settings, "sync");
  cJSON *obj_person    = cJSON_GetObjectItem(json_settings, "person");
  cJSON *obj_secret    = cJSON_GetObjectItem(json_settings, "secret");
  cJSON *obj_display   = cJSON_GetObjectItem(json_settings, "display");

  const char *badge_name   = json_get_str_value(obj_badge, "name");   // should be empty
  const char *ap_ssid      = json_get_str_value(obj_ap, "ssid");      // should be empty
  const char *ap_password  = json_get_str_value(obj_ap, "password");  // should be empty
  const char *web_login    = json_get_str_value(obj_web, "login");
  const char *sta_ssid     = json_get_str_value(obj_sta, "ssid");
  const char *sta_password = json_get_str_value(obj_sta, "password");
  const char *sync_path    = json_get_str_value(obj_sync, "path");

  // Update badge object
  esp_efuse_mac_get_default(badge_obj.mac);
  badge_obj.short_mac = (badge_obj.mac[4] << 8) + badge_obj.mac[5];
  if (default_loaded) {
    badge_obj.device_id = badge_generate_id();
    snprintf(badge_obj.device_name, SIZEOF(badge_obj.device_name), "Saiyan-%04x", badge_obj.short_mac);
    snprintf(badge_obj.ap_ssid, SIZEOF(badge_obj.ap_ssid), "Saiyan-%04x", badge_obj.short_mac);
    snprintf(badge_obj.ap_password, SIZEOF(badge_obj.ap_password), "%02x%02x%02x%02x", badge_obj.mac[2], badge_obj.mac[3], badge_obj.mac[4], badge_obj.mac[5]);
  } else {
    badge_obj.device_id = (int)cJSON_GetObjectItem(obj_badge, "id")->valuedouble;
    snprintf(badge_obj.device_name, SIZEOF(badge_obj.device_name), "%s", badge_name);
    snprintf(badge_obj.ap_ssid, SIZEOF(badge_obj.ap_ssid), "%s", ap_ssid);
    snprintf(badge_obj.ap_password, SIZEOF(badge_obj.ap_password), "%s", ap_password);
  }

  snprintf(badge_obj.web_login, SIZEOF(badge_obj.web_login), "%s", web_login);
  snprintf(badge_obj.sta_ssid, SIZEOF(badge_obj.sta_ssid), "%s", sta_ssid);
  snprintf(badge_obj.sta_password, SIZEOF(badge_obj.sta_password), "%s", sta_password);
  snprintf(badge_obj.sync_path, SIZEOF(badge_obj.sync_path), "%s", sync_path);

  badge_get_person_from_json(obj_person, &badge_obj.person);
  badge_get_person_from_json(obj_secret, &badge_obj.secret);

  // set brightness  with defaults if not present
  badge_obj.brightness_max = obj_display ? (uint8_t)json_get_int_value(obj_display, "brightness_max") : 255;
  badge_obj.brightness_mid = obj_display ? (uint8_t)json_get_int_value(obj_display, "brightness_mid") : 200;
  badge_obj.brightness_off = obj_display ? (uint8_t)json_get_int_value(obj_display, "brightness_off") : 0;

  ESP_LOGI(__FILE__, "Setting file loaded");
  ESP_LOGI(__FILE__, "The badge ID is: %d", badge_obj.device_id);

  if (default_loaded) {
    ESP_LOGI(__FILE__, "Saving settings file");
    cJSON_AddNumberToObject(obj_badge, "id", (double)badge_obj.device_id);
    badge_save_to_settings_file(json_settings);
    ESP_LOGI(__FILE__, "Setting file saved to default values");
  }
  cJSON_Delete(json_settings);
  // Update settings object
}
