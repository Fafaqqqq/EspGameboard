#include "wifi_init.h"

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "nvs_flash.h"

#define GAMEBOARD_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define GAMEBOARD_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define GAMEBOARD_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define GAMEBOARD_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

static const char* WIFI_TAG = "wifi_init";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
  if (event_id == WIFI_EVENT_AP_STACONNECTED)
  {
    wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*)event_data;
    ESP_LOGI(WIFI_TAG, "station join, AID=%d", event->aid);
  }
  else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
  {
    wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*)event_data;
    ESP_LOGI(WIFI_TAG, "station leave, AID=%d", event->aid);
  }
}

void wifi_pre_init()
{
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  ESP_ERROR_CHECK(ret);
}

void wifi_init()
{
  wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_create_default_wifi_ap();

  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

  wifi_config_t wifi_config = {
    .ap = {
      .ssid = GAMEBOARD_ESP_WIFI_SSID,
      .ssid_len = strlen(GAMEBOARD_ESP_WIFI_SSID),
      .channel = GAMEBOARD_ESP_WIFI_CHANNEL,
      .password = GAMEBOARD_ESP_WIFI_PASS,
      .max_connection = GAMEBOARD_MAX_STA_CONN,
      .authmode = WIFI_AUTH_WPA2_PSK,
      .pmf_cfg = {
        .required = true,
      },
    },
  };

  if (strlen(GAMEBOARD_ESP_WIFI_PASS) == 0)
  {
      wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(WIFI_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
            GAMEBOARD_ESP_WIFI_SSID, GAMEBOARD_ESP_WIFI_PASS, GAMEBOARD_ESP_WIFI_CHANNEL);
  return 0;
}