#include "wifi.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "lwip/ip6_addr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include <string.h>
#include <sdkconfig.h>

#define GAMEBOARD_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define GAMEBOARD_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define GAMEBOARD_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define GAMEBOARD_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#define HOST_IP_ADDR "192.168.4.1"
#define PORT 3333
// #define BUTTON_GPIO_PIN     14  // номер GPIO-пина, к которому подключена кнопка
// #define BUTTON_ACTIVE_LEVEL 0   // уровень логической активности кнопки

static const char* WIFI_TAG = "wifi_init";

#define TRX_BUF_SIZE 128
#define ADDR_BUF_SIZE 128

static char addr_str[ADDR_BUF_SIZE];
static char tx_buffer[TRX_BUF_SIZE];
static char rx_buffer[TRX_BUF_SIZE];

static volatile int32_t tx_size = 0;
static volatile int32_t rx_size = 0;

static SemaphoreHandle_t access_mtx = NULL;

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

void tcp_server_task(void *pvParameters)
{
  int err = 0;
  int listen_sock = 0;
  int accept_sock = 0;
  int addr_family = AF_INET;
  int ip_protocol = IPPROTO_IP;

  struct sockaddr_in dest_addr;
  struct sockaddr_in source_addr;

  socklen_t addr_len = 0;

  dest_addr.sin_family      = AF_INET;
  dest_addr.sin_port        = htons(PORT);
  dest_addr.sin_addr.s_addr = INADDR_ANY;

  inet_ntoa_r(dest_addr.sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
  ESP_LOGI(WIFI_TAG, "Pinned host addres: %s", addr_str);

  listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
  if (listen_sock < 0) {
      ESP_LOGE(WIFI_TAG, "Unable to create socket: errno %d", errno);
      vTaskDelete(NULL);
      return;
  }
  ESP_LOGI(WIFI_TAG, "Socket created");

  err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err != 0) {
    ESP_LOGE(WIFI_TAG, "Socket unable to bind: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }
  ESP_LOGI(WIFI_TAG, "Socket bound, port %d", PORT);

  err = listen(listen_sock, 1);
  if (err != 0) {
    ESP_LOGE(WIFI_TAG, "Error occurred during listen: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }
  ESP_LOGI(WIFI_TAG, "Socket listening");

  addr_len    = sizeof(source_addr);
  accept_sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
  if (accept_sock < 0)
  {
    ESP_LOGE(WIFI_TAG, "Unable to accept connection: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }

  inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
  ESP_LOGI(WIFI_TAG, "Socket accepted ip address: %s", addr_str);

  while (1)
  {
    if (xSemaphoreTake(access_mtx, pdMS_TO_TICKS(50)) != pdTRUE) continue;
    rx_size = recv(accept_sock, rx_buffer, TRX_BUF_SIZE, 0);
    xSemaphoreGive(access_mtx);

    if (0 > rx_size)
    {
      ESP_LOGE(WIFI_TAG, "Error occurred during receiving: errno %d", errno);
      break;
    } 
    else if (0 == rx_size)
    {
      ESP_LOGW(WIFI_TAG, "Connection closed");
      break;
    } 
    else
    {
      if (xSemaphoreTake(access_mtx, pdMS_TO_TICKS(50)) != pdTRUE) continue;
      int err = send(accept_sock, tx_buffer, tx_size, 0);
      // ESP_LOGI(WIFI_TAG, "send %ld bytes", tx_size);
      xSemaphoreGive(access_mtx);

      if (err < 0)
      {
          ESP_LOGE(WIFI_TAG, "Error occurred during sending: errno %d", errno);
          break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }

  ESP_LOGI(WIFI_TAG, "Closed");

  vTaskDelete(NULL);
}

int32_t wifi_get_rx(void* data, uint32_t* size)
{
  if (xSemaphoreTake(access_mtx, pdMS_TO_TICKS(50)) != pdTRUE)
  {
    return -__LINE__;
  }

  *size = rx_size;
  memcpy(data, rx_buffer, rx_size);
  
  xSemaphoreGive(access_mtx);
  return 0;
}

int32_t wifi_set_tx(const void* data, uint32_t size)
{
  if (xSemaphoreTake(access_mtx, pdMS_TO_TICKS(50)) != pdTRUE)
  {
    return -__LINE__;
  }

  tx_size = size;
  memcpy(tx_buffer, data, size);
  
  xSemaphoreGive(access_mtx);
  return 0;
}

void wifi_init()
{
  access_mtx = xSemaphoreCreateMutex();
  memset(tx_buffer, 0, TRX_BUF_SIZE);
  memset(rx_buffer, 0, TRX_BUF_SIZE);
  
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  ESP_ERROR_CHECK(ret);

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

  xTaskCreatePinnedToCore(tcp_server_task, "tcp server task", 4096, NULL, 3, NULL, 1);
}
