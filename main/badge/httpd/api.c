#include "api.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "session.h"

#include "httpd.h"
#include "../bt.h"
#include "../ui/person.h"
#include "../schedule.h"

#define is_string_match(a, c) (!strncmp(a, c, sizeof(c)))

static esp_err_t httpd_api_send_response(httpd_req_t *req, char *response) {
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  if (httpd_resp_sendstr(req, response) != ESP_OK) {
    ESP_LOGE(__FILE__, "Response failed!");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send response");
    ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
    return ESP_FAIL;
  }
  ESP_LOGI(__FILE__, "free_heap_size = %lu\n", esp_get_free_heap_size());
  return ESP_OK;
}

static esp_err_t httpd_api_system_info_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");

  cJSON *response = cJSON_CreateObject();

  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  cJSON_AddStringToObject(response, "IDF version", IDF_VER);
  cJSON_AddNumberToObject(response, "# cores", chip_info.cores);
  cJSON_AddStringToObject(response, "firmware branch", GIT_BRANCH);
  cJSON_AddStringToObject(response, "firmware commit", GIT_REV);
  cJSON_AddStringToObject(response, "firmware tag", GIT_TAG);

  char *response_str = cJSON_PrintUnformatted(response);

  esp_err_t err = httpd_api_send_response(req, response_str);

  cJSON_free((void *)response_str);
  cJSON_Delete(response);
  return err;
}

static esp_err_t httpd_api_schedule_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  const char *buf = schedule_load_from_file();

  if (!buf) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed on load_schedule_from_file()");
    return ESP_FAIL;
  }

  esp_err_t err = httpd_api_send_response(req, (char *)buf);
  free((char *)buf);
  return err;
}

static esp_err_t httpd_api_login_handler(httpd_req_t *req, const char *client_data) {
  httpd_resp_set_type(req, "application/json");

  cJSON *response    = cJSON_CreateObject();
  cJSON *client_json = cJSON_Parse(client_data);

  cJSON *client_pass = cJSON_GetObjectItem(client_json, "password");
  char *badge_pass   = badge_obj.web_login;
  esp_err_t err;
  if (cJSON_IsString(client_pass) && (client_pass->valuestring != NULL) && !strncmp(client_pass->valuestring, badge_pass, strlen(badge_pass))) {
    session_init(req);

    cJSON_AddStringToObject(response, "key", get_session_key());
    char *response_str = cJSON_PrintUnformatted(response);

    err = httpd_api_send_response(req, response_str);

    cJSON_free((void *)response_str);
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed on httpd_api_login_handler() function");
    err = ESP_FAIL;
  }

  cJSON_Delete(response);
  cJSON_Delete(client_json);
  return err;
}

static esp_err_t httpd_api_logout_handler(httpd_req_t *req, const char *client_data) {
  httpd_resp_set_type(req, "application/json");

  cJSON *response = cJSON_CreateObject();

  esp_err_t err;
  if (session_check(req, client_data)) {
    session_destroy();

    char *response_str = cJSON_PrintUnformatted(response);

    err = httpd_api_send_response(req, response_str);

    cJSON_free((void *)response_str);
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed on httpd_api_logout_handler() function");
    err = ESP_FAIL;
  }

  cJSON_Delete(response);

  return err;
}

static esp_err_t httpd_api_check_auth_handler(httpd_req_t *req, const char *client_data) {
  httpd_resp_set_type(req, "application/json");

  cJSON *response = cJSON_CreateObject();

  esp_err_t err;
  if (session_check(req, client_data)) {
    char *response_str = cJSON_PrintUnformatted(response);

    err = httpd_api_send_response(req, response_str);

    cJSON_free((void *)response_str);
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed on httpd_api_check_auth_handler() function");
    err = ESP_FAIL;
  }

  cJSON_Delete(response);

  return err;
}

static esp_err_t httpd_api_radar_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  cJSON *response = cJSON_CreateObject();
  cJSON *radar    = cJSON_AddArrayToObject(response, "radar");

  char buf[BADGE_BUF_SIZE] = {0};

  for (int i = 0; i < MAX_NEARBY_NODE; i++) {
    if (!ble_nodes[i].active) continue;

    cJSON *item = cJSON_CreateObject();

    // snprintf(buf, sizeof(buf), "%s", ble_nodes[i].name);
    cJSON_AddStringToObject(item, "name", ble_nodes[i].name);
    snprintf(buf, sizeof(buf), "%d dBm", ble_nodes[i].rssi);
    cJSON_AddStringToObject(item, "rssi", buf);
    snprintf(buf, sizeof(buf), "%d", ble_nodes[i].id);
    cJSON_AddStringToObject(item, "id", buf);
    cJSON_AddItemToArray(radar, item);
  }

  char *response_str = cJSON_PrintUnformatted(response);

  esp_err_t err = httpd_api_send_response(req, response_str);
  cJSON_free((void *)response_str);
  cJSON_Delete(response);
  return err;
}

static esp_err_t httpd_api_badge_name_handler(httpd_req_t *req, const char *client_data) {
  httpd_resp_set_type(req, "application/json");

  cJSON *response    = cJSON_CreateObject();
  cJSON *client_json = cJSON_Parse(client_data);

  esp_err_t err;
  if (session_check(req, client_data)) {
    cJSON *name = cJSON_GetObjectItem(client_json, "name");
    if (cJSON_IsString(name) && (name->valuestring != NULL)) {
      if (strlen(name->valuestring) > 0) {
        badge_update_attribute(DEVICE_NAME_ID, name->valuestring);
      }
    }
    cJSON_AddStringToObject(response, "name", badge_obj.device_name);
    char *response_str = cJSON_PrintUnformatted(response);

    err = httpd_api_send_response(req, response_str);

    cJSON_free((void *)response_str);
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed on httpd_api_badge_name_handler() function");
    err = ESP_FAIL;
  }

  cJSON_Delete(response);
  cJSON_Delete(client_json);

  return err;
}

static esp_err_t httpd_api_person_handler(httpd_req_t *req, const char *client_data) {
  httpd_resp_set_type(req, "application/json");

  cJSON *response    = cJSON_CreateObject();
  cJSON *client_json = cJSON_Parse(client_data);

  esp_err_t err;
  if (session_check(req, client_data)) {
    cJSON *person = cJSON_GetObjectItem(client_json, "person");
    if (cJSON_IsObject(person)) {
      cJSON *name         = cJSON_GetObjectItem(person, "name");
      cJSON *organization = cJSON_GetObjectItem(person, "organization");
      cJSON *job          = cJSON_GetObjectItem(person, "job");
      cJSON *message      = cJSON_GetObjectItem(person, "message");
      cJSON *url          = cJSON_GetObjectItem(person, "url");

      if (cJSON_IsString(name) && name->valuestring != NULL && strlen(name->valuestring) > 0)
        badge_update_attribute(PERSON_NAME_ID, name->valuestring);
      if (cJSON_IsString(organization) && organization->valuestring != NULL && strlen(organization->valuestring) > 0)
        badge_update_attribute(PERSON_ORGANIZATION_ID, organization->valuestring);
      if (cJSON_IsString(job) && job->valuestring != NULL && strlen(job->valuestring) > 0)
        badge_update_attribute(PERSON_JOB_ID, job->valuestring);
      if (cJSON_IsString(message) && message->valuestring != NULL && strlen(message->valuestring) > 0)
        badge_update_attribute(PERSON_MESSAGE_ID, message->valuestring);
      if (cJSON_IsString(url) && url->valuestring != NULL && strlen(url->valuestring) > 0)
        badge_update_attribute(PERSON_URL_ID, url->valuestring);

      ui_person_set_secret(false);
    }
    cJSON *person_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(person_obj, "name", badge_obj.person.name);
    cJSON_AddStringToObject(person_obj, "organization", badge_obj.person.organization);
    cJSON_AddStringToObject(person_obj, "job", badge_obj.person.job);
    cJSON_AddStringToObject(person_obj, "message", badge_obj.person.message);
    cJSON_AddStringToObject(person_obj, "url", badge_obj.person.url);
    cJSON_AddItemToObject(response, "person", person_obj);

    char *response_str = cJSON_PrintUnformatted(response);

    err = httpd_api_send_response(req, response_str);

    cJSON_free((void *)response_str);
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed on httpd_api_person_handler() function");
    err = ESP_FAIL;
  }

  cJSON_Delete(response);
  cJSON_Delete(client_json);

  return err;
}

static esp_err_t httpd_api_secret_handler(httpd_req_t *req, const char *client_data) {
  httpd_resp_set_type(req, "application/json");

  cJSON *response    = cJSON_CreateObject();
  cJSON *client_json = cJSON_Parse(client_data);

  esp_err_t err;
  if (session_check(req, client_data)) {
    cJSON *secret = cJSON_GetObjectItem(client_json, "secret");
    if (cJSON_IsObject(secret)) {
      cJSON *name         = cJSON_GetObjectItem(secret, "name");
      cJSON *organization = cJSON_GetObjectItem(secret, "organization");
      cJSON *job          = cJSON_GetObjectItem(secret, "job");
      cJSON *message      = cJSON_GetObjectItem(secret, "message");
      cJSON *url          = cJSON_GetObjectItem(secret, "url");

      if (cJSON_IsString(name) && name->valuestring != NULL && strlen(name->valuestring) > 0)
        badge_update_attribute(SECRET_NAME_ID, name->valuestring);
      if (cJSON_IsString(organization) && organization->valuestring != NULL && strlen(organization->valuestring) > 0)
        badge_update_attribute(SECRET_ORGANIZATION_ID, organization->valuestring);
      if (cJSON_IsString(job) && job->valuestring != NULL && strlen(job->valuestring) > 0)
        badge_update_attribute(SECRET_JOB_ID, job->valuestring);
      if (cJSON_IsString(message) && message->valuestring != NULL && strlen(message->valuestring) > 0)
        badge_update_attribute(SECRET_MESSAGE_ID, message->valuestring);
      if (cJSON_IsString(url) && url->valuestring != NULL && strlen(url->valuestring) > 0)
        badge_update_attribute(SECRET_URL_ID, url->valuestring);

      ui_person_set_secret(true);
    }
    cJSON *secret_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(secret_obj, "name", badge_obj.secret.name);
    cJSON_AddStringToObject(secret_obj, "organization", badge_obj.secret.organization);
    cJSON_AddStringToObject(secret_obj, "job", badge_obj.secret.job);
    cJSON_AddStringToObject(secret_obj, "message", badge_obj.secret.message);
    cJSON_AddStringToObject(secret_obj, "url", badge_obj.secret.url);
    cJSON_AddItemToObject(response, "secret", secret_obj);

    char *response_str = cJSON_PrintUnformatted(response);

    err = httpd_api_send_response(req, response_str);

    cJSON_free((void *)response_str);
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed on httpd_api_secret_handler() function");
    err = ESP_FAIL;
  }

  cJSON_Delete(response);
  cJSON_Delete(client_json);

  return err;
}

static esp_err_t httpd_api_wifi_handler(httpd_req_t *req, const char *client_data) {
  httpd_resp_set_type(req, "application/json");

  cJSON *response    = cJSON_CreateObject();
  cJSON *client_json = cJSON_Parse(client_data);

  esp_err_t err;
  if (session_check(req, client_data)) {
    cJSON *wifi = cJSON_GetObjectItem(client_json, "wifi");
    if (cJSON_IsObject(wifi)) {
      cJSON *ssid     = cJSON_GetObjectItem(wifi, "ssid");
      cJSON *password = cJSON_GetObjectItem(wifi, "password");

      if (cJSON_IsString(ssid) && (ssid->valuestring != NULL) && (strlen(ssid->valuestring) > 0)) {
        badge_update_attribute(WIFI_SSID_ID, ssid->valuestring);
      } else if (cJSON_IsString(password) && (password->valuestring != NULL) && (strlen(password->valuestring) > 0)) {
        badge_update_attribute(DEVICE_NAME_ID, password->valuestring);
      }
    }

    cJSON *wifi_obj = cJSON_CreateObject();
    cJSON_AddStringToObject(wifi_obj, "ssid", badge_obj.ap_ssid);
    cJSON_AddStringToObject(wifi_obj, "password", badge_obj.ap_password);
    cJSON_AddItemToObject(response, "wifi", wifi_obj);

    char *response_str = cJSON_PrintUnformatted(response);

    err = httpd_api_send_response(req, response_str);

    cJSON_free((void *)response_str);

  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed on httpd_api_wifi_handler() function");
    err = ESP_FAIL;
  }

  cJSON_Delete(response);
  cJSON_Delete(client_json);

  return err;
}

static esp_err_t httpd_api_password_handler(httpd_req_t *req, const char *client_data) {
  httpd_resp_set_type(req, "application/json");

  cJSON *response = cJSON_CreateObject();

  cJSON *client_json = cJSON_Parse(client_data);

  esp_err_t err;
  if (session_check(req, client_data)) {
    cJSON *password = cJSON_GetObjectItem(client_json, "password");
    if (cJSON_IsString(password) && (password->valuestring != NULL) && (strlen(password->valuestring) > 0)) {
      badge_update_attribute(WEB_LOGIN_ID, password->valuestring);
    }
    char *response_str = cJSON_PrintUnformatted(response);

    err = httpd_api_send_response(req, response_str);

    cJSON_free((void *)response_str);
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed on httpd_api_password_handler() function");
    err = ESP_FAIL;
  }

  cJSON_Delete(response);
  cJSON_Delete(client_json);

  return err;
}

static void reset_timer_callback(void *arg) {
  esp_restart();
}

static esp_err_t httpd_api_reset_handler(httpd_req_t *req, const char *client_data) {
  httpd_resp_set_type(req, "application/json");

  cJSON *response = cJSON_CreateObject();

  esp_err_t err;
  if (session_check(req, client_data)) {
    char *response_str = cJSON_PrintUnformatted(response);

    err = httpd_api_send_response(req, response_str);

    cJSON_free((void *)response_str);

    if (!unlink(SETTINGS_FILE)) {
      esp_timer_handle_t reset_timer;
      const esp_timer_create_args_t timer_args = {
          .callback = &reset_timer_callback,
          .name     = "reset-timer"};

      ESP_ERROR_CHECK(esp_timer_create(&timer_args, &reset_timer));
      esp_timer_start_once(reset_timer, 3 * 1000000);
    }
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed on httpd_api_reset_handler() function");
    err = ESP_FAIL;
  }

  cJSON_Delete(response);

  return err;
}

esp_err_t httpd_api_handler(httpd_req_t *req) {
  int total_len = req->content_len;
  int cur_len   = 0;
  char buf[SCRATCH_BUFSIZE];
  int received = 0;

  if (total_len >= SCRATCH_BUFSIZE) {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
    return ESP_FAIL;
  }
  while (cur_len < total_len) {
    received = httpd_req_recv(req, buf + cur_len, total_len);
    if (received <= 0) {
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
      return ESP_FAIL;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';

  char *cmd = strstr(req->uri, API_ENDPOINT);
  if (cmd != NULL)
    cmd += strlen(API_ENDPOINT);
  ESP_LOGI(__FILE__, "command: %s", cmd);

  if (is_string_match(cmd, "login")) {
    httpd_api_login_handler(req, buf);
  } else if (is_string_match(cmd, "logout")) {
    httpd_api_logout_handler(req, buf);
  } else if (is_string_match(cmd, "check_authentication")) {
    httpd_api_check_auth_handler(req, buf);
  } else if (is_string_match(cmd, "schedule")) {
    httpd_api_schedule_handler(req);
  } else if (is_string_match(cmd, "info")) {
    httpd_api_system_info_handler(req);
  } else if (is_string_match(cmd, "radar")) {
    httpd_api_radar_handler(req);
  } else if (is_string_match(cmd, "name")) {
    httpd_api_badge_name_handler(req, buf);
  } else if (is_string_match(cmd, "person")) {
    httpd_api_person_handler(req, buf);
  } else if (is_string_match(cmd, "secret")) {
    httpd_api_secret_handler(req, buf);
  } else if (is_string_match(cmd, "wifi")) {
    httpd_api_wifi_handler(req, buf);
  } else if (is_string_match(cmd, "password")) {
    httpd_api_password_handler(req, buf);
  } else if (is_string_match(cmd, "reset")) {
    httpd_api_reset_handler(req, buf);
  } else {
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "not found");
  }

  return ESP_OK;
}
