#include "session.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"

static char session_key[SESSION_KEY_LEN + 1];

void byte_to_hex_str(char *xp, const char *bb, int n) {
  const char xx[] = "0123456789ABCDEF";
  while (--n >= 0)
    xp[n] = xx[(bb[n >> 1] >> ((1 - (n & 1)) << 2)) & 0xF];
}

const char *get_session_key() {
  return session_key;
}

void session_destroy() {
  ESP_LOGI(__FILE__, "Destroy Context function called");
  session_key[0] = '\0';
}

esp_err_t session_init(httpd_req_t *req) {
  char nonce[SESSION_KEY_LEN];
  esp_fill_random(nonce, SESSION_KEY_LEN);
  byte_to_hex_str(session_key, nonce, SESSION_KEY_LEN);

  return ESP_OK;
}

bool check_session(httpd_req_t *req, const char *client_data) {
  if (!session_key[0]) return false;
  if (!client_data) return false;

  cJSON *client_json = cJSON_Parse(client_data);
  cJSON *client_key  = cJSON_GetObjectItem(client_json, "key");

  if (cJSON_IsString(client_key) && (client_key->valuestring != NULL)) {
    ESP_LOGE(__FILE__, "Session key: %s", session_key);
    ESP_LOGE(__FILE__, "Client key: %s", client_key->valuestring);

    if (cJSON_IsString(client_key) && (client_key->valuestring != NULL)) {
      bool res = !strncmp(client_key->valuestring, session_key, SESSION_KEY_LEN);
      cJSON_Delete(client_json);
      return res;
    }
  }
  cJSON_Delete(client_json);
  return false;
}
