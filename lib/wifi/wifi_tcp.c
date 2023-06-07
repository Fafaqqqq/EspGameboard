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

#define PORT 3333
#define HOST_IP_ADDR "192.168.4.1"

#define  TRX_BUF_SIZE 128
#define ADDR_BUF_SIZE 128

typedef socket_t get_socket_func();

static const char* WIFI_TAG = "wifi_tcp";

static char addr_str[ADDR_BUF_SIZE];

static SemaphoreHandle_t accept_mtx_ = NULL;
static TaskHandle_t tcp_task_handle_ = NULL;

static QueueHandle_t rx_queue = NULL;
static QueueHandle_t tx_queue = NULL;

static volatile int32_t g_tx_size = 0;
static volatile int32_t g_rx_size = 0;

static void tcp_task
(
  void *pvParameters
)
{
  char tx_buffer[TRX_BUF_SIZE];
  char rx_buffer[TRX_BUF_SIZE];
  
  int tcp_type = (int)pvParameters;

  ESP_LOGI(WIFI_TAG, "tcp type %d", tcp_type);

  socket_t sock  = tcp_type ? wifi_accept() : wifi_connect();

  xSemaphoreGive(accept_mtx_);

  while (1)
  {
    if (xQueueReceive(tx_queue, tx_buffer, pdMS_TO_TICKS(20)) != pdTRUE)
      continue;

    int err = send(sock, tx_buffer, g_tx_size, 0);

    if (0 > err)
    {
      ESP_LOGE(WIFI_TAG, "Error occurred during receiving: errno %d", errno);
      break;
    } 
    else if (0 == err)
    {
      ESP_LOGW(WIFI_TAG, "Connection closed");
      break;
    } 
    else
    {
      g_rx_size = recv(sock, rx_buffer, TRX_BUF_SIZE, 0);

      if (g_rx_size < 0)
      {
          ESP_LOGE(WIFI_TAG, "Error occurred during sending: errno %d", errno);
          break;
      }

      xQueueSend(rx_queue, rx_buffer, pdMS_TO_TICKS(20));
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }

  ESP_LOGI(WIFI_TAG, "Closed");

  vTaskDelete(NULL);
}

socket_t wifi_accept()
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
      return -__LINE__;
  }
  ESP_LOGI(WIFI_TAG, "Socket created");

  err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err != 0) {
    ESP_LOGE(WIFI_TAG, "Socket unable to bind: errno %d", errno);
    vTaskDelete(NULL);
    return -__LINE__;
  }
  ESP_LOGI(WIFI_TAG, "Socket bound, port %d", PORT);

  err = listen(listen_sock, 1);
  if (err != 0) {
    ESP_LOGE(WIFI_TAG, "Error occurred during listen: errno %d", errno);
    vTaskDelete(NULL);
    return -__LINE__;
  }

  ESP_LOGI(WIFI_TAG, "Socket listening");

  addr_len    = sizeof(source_addr);
  accept_sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
  if (accept_sock < 0)
  {
    ESP_LOGE(WIFI_TAG, "Unable to accept connection: errno %d", errno);
    vTaskDelete(NULL);
    return -__LINE__;
  }

  int nodelay = 1;
  setsockopt(accept_sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));


  return accept_sock;
}

socket_t wifi_connect()
{
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
    return -__LINE__;
  }
  ESP_LOGI(WIFI_TAG, "Socket created");

  err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err != 0)
  {
    ESP_LOGE(WIFI_TAG, "Socket unable to connect: errno %d", errno);
    vTaskDelete(NULL);
    return -__LINE__;
  }
  ESP_LOGI(WIFI_TAG, "Successfully connected to server");

  int nodelay = 1;
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

  return sock;
}

int32_t wifi_transmit(socket_t sock, const void* local, int32_t size)
{
  int err = send(sock, local, size, 0);

  if (err < 0)
  {
    ESP_LOGE(WIFI_TAG, "Error occurred during sending: errno %d", errno);
    return -__LINE__;
  }

  return 0;
}

int32_t wifi_recieve(socket_t sock, void* local, int32_t* size, uint32_t max_size)
{
  *size = recv(sock, local, max_size, 0);

  if (0 > *size)
  {
    ESP_LOGE(WIFI_TAG, "Error occurred during receiving: errno %d", errno);
    return -__LINE__;
  } 
  else if (0 == *size)
  {
    ESP_LOGW(WIFI_TAG, "Connection closed");
    return -__LINE__;
  }

  return 0;
}

int32_t wifi_start_tcp
(
  uint32_t tx_size,
  uint32_t rx_size,
  int      tcp_type
)
{
  if (tcp_type < 0 || tcp_type > 1)
    return -__LINE__;

  if (rx_queue || tx_queue || accept_mtx_)
    return -__LINE__;

  accept_mtx_ = xSemaphoreCreateBinary();
  ESP_LOGI(WIFI_TAG, "start tcp");

  g_rx_size = rx_size;
  g_tx_size = tx_size;

  rx_queue = xQueueCreate(128, rx_size);
  tx_queue = xQueueCreate(128, tx_size);

  xTaskCreatePinnedToCore(tcp_task, "tcp_task", 4096, (void*)tcp_type, 1, &tcp_task_handle_, 1);

  xSemaphoreTake(accept_mtx_, portMAX_DELAY);

  return 0;
}

int32_t wifi_stop_tcp
(
  uint32_t tx_size,
  uint32_t rx_size
)
{
  return 0;
}

int32_t wifi_get_rx
(
  void* data
)
{
  return rx_queue 
         ? xQueueReceive(rx_queue, data, pdMS_TO_TICKS(0))
         : -__LINE__;
}

int32_t wifi_set_tx
(
  const void* data
)
{
  return tx_queue
         ? xQueueSend(tx_queue, data, pdTICKS_TO_MS(20))
         : -__LINE__;
}