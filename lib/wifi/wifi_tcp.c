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

static const char* WIFI_TAG = "wifi_tcp";

static char addr_str[ADDR_BUF_SIZE];

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