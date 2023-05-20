#include "wifi.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_mac.h"

#include "lwip/ip6_addr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <string.h>
#include <sdkconfig.h>

#define GAMEBOARD_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define GAMEBOARD_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define GAMEBOARD_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define GAMEBOARD_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK

#define HOST_IP_ADDR "192.168.4.1"
#define PORT 3333

static const char* WIFI_TAG = "wifi_init";

#define TRX_BUF_SIZE 128
#define ADDR_BUF_SIZE 128
#define MAX_AVAILEBLE_NETS 10
#define MAX_SSID_LENGTH 33

static char addr_str[ADDR_BUF_SIZE];
static char tx_buffer[TRX_BUF_SIZE];
static char rx_buffer[TRX_BUF_SIZE];

static volatile int32_t tx_size = 0;
static volatile int32_t rx_size = 0;

static char* availeble_nets_[MAX_AVAILEBLE_NETS];
static int s_retry_num = 0;

static SemaphoreHandle_t access_mtx_ = NULL;
static SemaphoreHandle_t accept_mtx_ = NULL;
static TaskHandle_t tcp_task_handle_ = NULL;

static EventGroupHandle_t s_wifi_event_group = NULL;

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
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
  {
    esp_wifi_connect();
  }
  else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
    {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
    }
    else
    {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(WIFI_TAG, "connect to the AP fail");
  }
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

const char** wifi_scan_esp_net(uint32_t* net_cnt)
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
  assert(sta_netif);

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  uint16_t number   = MAX_AVAILEBLE_NETS;
  uint16_t ap_count = 0;
  wifi_ap_record_t ap_info[MAX_AVAILEBLE_NETS];
  memset(ap_info, 0, sizeof(ap_info));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  esp_wifi_scan_start(NULL, true);

  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

  uint32_t esp_net_cnt = 0;

  for (int i = 0; (i < MAX_AVAILEBLE_NETS) && (i < ap_count); i++) {
    
    if (memcmp(ap_info[i].ssid, GAMEBOARD_ESP_WIFI_SSID, strlen(GAMEBOARD_ESP_WIFI_SSID)) == 0)
    {
      strcpy(availeble_nets_[esp_net_cnt++], (const char*)(ap_info[i].ssid));
    }
  }

  ESP_ERROR_CHECK(esp_wifi_stop());
  ESP_ERROR_CHECK(esp_wifi_deinit());
  ESP_ERROR_CHECK(esp_event_loop_delete_default());

  *net_cnt = esp_net_cnt;

  return esp_net_cnt ? availeble_nets_ : NULL;
}

static void tcp_server_task
(
  void *pvParameters
)
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
  inet_aton(HOST_IP_ADDR, &(dest_addr.sin_addr.s_addr));

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

  xSemaphoreGive(accept_mtx_);

  inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
  ESP_LOGI(WIFI_TAG, "Socket accepted ip address: %s", addr_str);

  while (1)
  {
    if (xSemaphoreTake(access_mtx_, pdMS_TO_TICKS(50)) != pdTRUE) continue;
    rx_size = recv(accept_sock, rx_buffer, TRX_BUF_SIZE, 0);
    xSemaphoreGive(access_mtx_);

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
      if (xSemaphoreTake(access_mtx_, pdMS_TO_TICKS(50)) != pdTRUE) continue;
      int err = send(accept_sock, tx_buffer, tx_size, 0);
      xSemaphoreGive(access_mtx_);

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

static void tcp_client_task
(
  void *pvParameters
)
{
  ESP_LOGI(WIFI_TAG, "tcp client start");
  int err = 0;
  struct sockaddr_in dest_addr = {0};

  int addr_family = AF_INET;
  int ip_protocol = IPPROTO_IP;

  dest_addr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(PORT);

  inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

  int sock = socket(addr_family, SOCK_STREAM, ip_protocol);
  if (sock < 0)
  {
    ESP_LOGE(WIFI_TAG, "Unable to create socket: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }
  ESP_LOGI(WIFI_TAG, "Socket created");

  err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err != 0)
  {
    ESP_LOGE(WIFI_TAG, "Socket unable to connect: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }
  ESP_LOGI(WIFI_TAG, "Successfully connected to server");

  // xSemaphoreGive(accept_mtx_);

  while (1)
  {
    if (xSemaphoreTake(access_mtx_, pdMS_TO_TICKS(20)) != pdTRUE) continue;
    int len = send(sock, tx_buffer, tx_size, 0);
    xSemaphoreGive(accept_mtx_);

    if (len < 0)
    {
      ESP_LOGE(WIFI_TAG, "Error occurred during sending: errno %d", errno);
      break;
    }
    else
    {
      // ESP_LOGI(WIFI_TAG, "Sent %d bytes: %s", len, tx_buffer);

      // Принимаем ответ
      if (xSemaphoreTake(access_mtx_, pdMS_TO_TICKS(20)) != pdTRUE) continue;
      len = recv(sock, rx_buffer, TRX_BUF_SIZE, 0);
      xSemaphoreGive(accept_mtx_);

      if (len < 0)
      {
        ESP_LOGE(WIFI_TAG, "Error occurred during receiving: errno %d", errno);
        break;
      }
      else
      {
        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
        // ESP_LOGI(WIFI_TAG, "Received %d bytes: %s", len, rx_buffer);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }

  ESP_LOGI(WIFI_TAG, "Closed");

  vTaskDelete(NULL);
}

int32_t wifi_get_rx
(
  void* data,
  uint32_t* size
)
{
  if (xSemaphoreTake(access_mtx_, pdMS_TO_TICKS(50)) != pdTRUE)
  {
    return -__LINE__;
  }

  *size = rx_size;
  memcpy(data, rx_buffer, rx_size);
  
  xSemaphoreGive(access_mtx_);
  return 0;
}

int32_t wifi_set_tx
(
  const void* data,
  uint32_t size
)
{
  if (xSemaphoreTake(access_mtx_, pdMS_TO_TICKS(50)) != pdTRUE)
  {
    return -__LINE__;
  }

  tx_size = size;
  memcpy(tx_buffer, data, size);
  
  xSemaphoreGive(access_mtx_);
  return 0;
}

void wifi_init()
{
  accept_mtx_ = xSemaphoreCreateBinary();
  access_mtx_ = xSemaphoreCreateMutex();


  memset(tx_buffer, 0, TRX_BUF_SIZE);
  memset(rx_buffer, 0, TRX_BUF_SIZE);
  
  // xSemaphoreTake(accept_mtx_, portMAX_DELAY);
  // xSemaphoreTake(access_mtx_, portMAX_DELAY);
  
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  for (uint32_t i = 0; i < MAX_AVAILEBLE_NETS; i++)
  {
    availeble_nets_[i] = (char*)malloc(MAX_SSID_LENGTH);
  }
}

void wifi_init_sta
(
  void *pvParams
)
{
  const char* net_ssid = (const char*)pvParams;

  memset(tx_buffer, 0, TRX_BUF_SIZE);
  memset(rx_buffer, 0, TRX_BUF_SIZE);

  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                      ESP_EVENT_ANY_ID,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                      IP_EVENT_STA_GOT_IP,
                                                      &wifi_event_handler,
                                                      NULL,
                                                      &instance_got_ip));

  wifi_config_t wifi_config = {
      .sta = {
          .password = EXAMPLE_ESP_WIFI_PASS,
          .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
          .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
      },
  };

  strcpy((char*)(wifi_config.sta.ssid), net_ssid);
  strcpy((char*)(wifi_config.sta.password), net_ssid);

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(WIFI_TAG, "wifi_init_sta finished.");

  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                         pdFALSE,
                                         pdFALSE,
                                         portMAX_DELAY);

  if (bits & WIFI_CONNECTED_BIT)
  {
    ESP_LOGI(WIFI_TAG, "connected to ap SSID:%s password:%s",
             net_ssid, EXAMPLE_ESP_WIFI_PASS);
  }
  else if (bits & WIFI_FAIL_BIT)
  {
    ESP_LOGI(WIFI_TAG, "Failed to connect to SSID:%s, password:%s",
             net_ssid, EXAMPLE_ESP_WIFI_PASS);
  }
  else
  {
    ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
  }

  // xTaskCreatePinnedToCore(tcp_client_task, "tcp client task", 4096, NULL, 1, &tcp_task_handle_, 1);

  // xSemaphoreTake(accept_mtx_, portMAX_DELAY);
}

void wifi_init_ap
(
  void *pvParams
)
{
  ESP_LOGI(WIFI_TAG, "wifi init ap");

  const char* net_ssid = (const char*)pvParams;

  memset(tx_buffer, 0, TRX_BUF_SIZE);
  memset(rx_buffer, 0, TRX_BUF_SIZE);
  
  wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_create_default_wifi_ap();

  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

  wifi_config_t wifi_config = {
    .ap = {
      .ssid_len = strlen(net_ssid),
      .channel = GAMEBOARD_ESP_WIFI_CHANNEL,
      .max_connection = GAMEBOARD_MAX_STA_CONN,
      .authmode = WIFI_AUTH_WPA2_PSK,
      .pmf_cfg = {
        .required = true,
      },
    },
  };

  strcpy((char*)(wifi_config.ap.ssid), net_ssid);
  strcpy((char*)(wifi_config.ap.password), net_ssid);

  if (strlen(GAMEBOARD_ESP_WIFI_PASS) == 0)
  {
      wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(WIFI_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
            GAMEBOARD_ESP_WIFI_SSID, GAMEBOARD_ESP_WIFI_PASS, GAMEBOARD_ESP_WIFI_CHANNEL);

  // xTaskCreatePinnedToCore(tcp_server_task, "tcp server task", 4096, NULL, 1, &tcp_task_handle_, 1);

  // xSemaphoreTake(accept_mtx_, portMAX_DELAY);
}
