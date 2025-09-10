#include "info.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "../badge.h"

void list_all_netifs() {
  ESP_LOGI(__FILE__, "=== LISTING ALL NETWORK INTERFACES ===");

  // Try to iterate through all available network interfaces
  esp_netif_t *netif      = NULL;
  esp_netif_t *temp_netif = esp_netif_next_unsafe(netif);
  int count               = 0;

  while (temp_netif != NULL) {
    count++;
    ESP_LOGI(__FILE__, "Found netif %d: %p", count, temp_netif);

    // Get interface description
    const char *desc = esp_netif_get_desc(temp_netif);
    ESP_LOGI(__FILE__, "Interface %d description: %s", count, desc ? desc : "unknown");

    // Get IP info for this interface
    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(temp_netif, &ip_info);
    ESP_LOGI(__FILE__, "Interface %d IP info (ret: %s):", count, esp_err_to_name(ret));
    ESP_LOGI(__FILE__, "  IP: " IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI(__FILE__, "  Gateway: " IPSTR, IP2STR(&ip_info.gw));
    ESP_LOGI(__FILE__, "  Netmask: " IPSTR, IP2STR(&ip_info.netmask));

    temp_netif = esp_netif_next_unsafe(temp_netif);
  }

  ESP_LOGI(__FILE__, "Total network interfaces found: %d", count);
  ESP_LOGI(__FILE__, "=== END NETIF LISTING ===");
}

static esp_err_t update_ap_sta_ip_info(wifi_info_t *wifi_info) {
  esp_netif_t *netif = NULL;
  esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

  wifi_mode_t wifi_mode;
  ESP_LOGI(__FILE__, "Checking wifi mode");
  ESP_ERROR_CHECK(esp_wifi_get_mode(&wifi_mode));
  switch (wifi_mode) {
    case WIFI_MODE_NULL:
      ESP_LOGI(__FILE__, "Wifi is in NULL mode");
      wifi_info->wifi_mode = WIFI_MODE_NULL;
      return ESP_FAIL;
      break;
    case WIFI_MODE_STA:
      ESP_LOGI(__FILE__, "Wifi is in STA mode");
      netif                = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
      wifi_info->wifi_mode = WIFI_MODE_STA;
      break;
    case WIFI_MODE_AP:
      ESP_LOGI(__FILE__, "Wifi is in AP mode");
      netif                = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
      wifi_info->wifi_mode = WIFI_MODE_AP;
      break;
    default:
      ESP_LOGE(__FILE__, "Wifi is in Unkown mode");
      wifi_info->wifi_mode = WIFI_MODE_NAN;
      return ESP_FAIL;
      break;
  }

  ESP_LOGI(__FILE__, "STA netif handle (WIFI_STA_DEF): %p", netif);

  if (!netif) {
    ESP_LOGW(__FILE__, "Could not get netif handle");
    return ESP_FAIL;
  }
  esp_netif_ip_info_t ip_info;
  esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
  ESP_LOGI(__FILE__, "esp_netif_get_ip_info returned: %s", esp_err_to_name(ret));
  ESP_LOGI(__FILE__, "IP: " IPSTR, IP2STR(&ip_info.ip));
  ESP_LOGI(__FILE__, "Gateway: " IPSTR, IP2STR(&ip_info.gw));
  ESP_LOGI(__FILE__, "Netmask: " IPSTR, IP2STR(&ip_info.netmask));

  if (ret == ESP_OK && ip_info.ip.addr != 0) {
    snprintf(wifi_info->ip, IP_BUFF_SIZE, IPSTR, IP2STR(&ip_info.ip));
    snprintf(wifi_info->gateway, IP_BUFF_SIZE, IPSTR, IP2STR(&ip_info.gw));
    snprintf(wifi_info->netmask, IP_BUFF_SIZE, IPSTR, IP2STR(&ip_info.netmask));
    return ESP_OK;
  }
  ESP_LOGW(__FILE__, "IP is not valid or error occurred. ret=%s, ip.addr=0x%08x", esp_err_to_name(ret), ip_info.ip.addr);
  return ESP_FAIL;
}

esp_err_t update_ip_info(wifi_info_t *wifi_info) {
  ESP_LOGI(__FILE__, "=== IP INFO DEBUG ===");

  // First, list all network interfaces for debugging
  list_all_netifs();
  if (update_ap_sta_ip_info(wifi_info) == ESP_OK)
    return ESP_OK;
  // If we reach here, we couldn't get IP info through normal methods
  // Try iterating through all interfaces as fallback
  ESP_LOGI(__FILE__, "Primary methods failed, trying to iterate through all interfaces...");

  esp_netif_t *netif      = NULL;
  esp_netif_t *temp_netif = esp_netif_next_unsafe(netif);
  wifi_info->wifi_mode    = WIFI_MODE_NULL;
  
  while (temp_netif != NULL) {
    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(temp_netif, &ip_info);

    if (ret == ESP_OK && ip_info.ip.addr != 0) {
      const char *desc = esp_netif_get_desc(temp_netif);
      ESP_LOGI(__FILE__, "Found valid IP on interface %s: " IPSTR, desc ? desc : "unknown", IP2STR(&ip_info.ip));

      // Use interface description to determine type instead of IP range
      // heuristic
      if (desc && (strstr(desc, "ap") || strstr(desc, "sta"))) {
        if (strstr(desc, "ap"))
          wifi_info->wifi_mode = WIFI_MODE_AP;
        else if (strstr(desc, "sta"))
          wifi_info->wifi_mode = WIFI_MODE_STA;

        // AP interface
        snprintf(wifi_info->ip, IP_BUFF_SIZE, IPSTR, IP2STR(&ip_info.ip));
        snprintf(wifi_info->gateway, IP_BUFF_SIZE, IPSTR, IP2STR(&ip_info.gw));
        snprintf(wifi_info->netmask, IP_BUFF_SIZE, IPSTR, IP2STR(&ip_info.netmask));
        return ESP_OK;
      }
      ESP_LOGI(__FILE__, "Successfully displayed IP info from interface iteration");
    }
    temp_netif = esp_netif_next_unsafe(temp_netif);
  }
  snprintf(wifi_info->ip, IP_BUFF_SIZE, "Not found");
  snprintf(wifi_info->gateway, IP_BUFF_SIZE, "Not found");
  snprintf(wifi_info->netmask, IP_BUFF_SIZE, "Not found");

  ESP_LOGI(__FILE__, "IP not found");
  return ESP_FAIL;
}