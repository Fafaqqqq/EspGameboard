#include <stdio.h>
#include <string.h>

#include "joystick.h"
#include "spi_display.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "nvs_flash.h"

#include "lwip/ip6_addr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#define HOST_IP_ADDR "192.168.4.1"
#define PORT 3333

const char *TAG = "master_target";

static volatile SemaphoreHandle_t client_sem = NULL;
static volatile SemaphoreHandle_t server_sem = NULL;


static volatile joystick_data_t client_stick_data;
static volatile joystick_data_t server_stick_data;

static volatile spi_device_handle_t device_handle;


void spi_pre_transfer_callback(spi_transaction_t* spi_trans)
{
  int dc = (int)spi_trans->user;
  gpio_set_level(CONFIG_PIN_NUM_DC, dc);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station join, AID=%d", event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station leave, AID=%d", event->aid);
    }
}

static void wifi_init_softap(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_ap();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config =
    {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                    .required = true,
            },
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

static void tcp_server_task(void *pvParameters)
{
  char tx_buffer[128];
  char rx_buffer[128];
  char addr_str[128];

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
  ESP_LOGI(TAG, "Pinned host addres: %s", addr_str);

  listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
  if (listen_sock < 0) {
      ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
      vTaskDelete(NULL);
      return;
  }
  ESP_LOGI(TAG, "Socket created");

  err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err != 0) {
    ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }
  ESP_LOGI(TAG, "Socket bound, port %d", PORT);

  err = listen(listen_sock, 1);
  if (err != 0) {
    ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }
  ESP_LOGI(TAG, "Socket listening");

  addr_len    = sizeof(source_addr);
  accept_sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
  if (accept_sock < 0)
  {
    ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
    vTaskDelete(NULL);
    return;
  }

  inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
  ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

  while (1)
  {
    int len = recv(accept_sock, rx_buffer, sizeof(joystick_data_t), 0);
    if (0 > len)
    {
      ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
      break;
    } 
    else if (0 == len)
    {
      ESP_LOGW(TAG, "Connection closed");
      break;
    } 
    else
    {
      // xSemaphoreTake(client_sem, pdMS_TO_TICKS(20));

      client_stick_data.x = ((joystick_data_t*)rx_buffer)->x;
      client_stick_data.y = ((joystick_data_t*)rx_buffer)->y;
      xSemaphoreGive(client_sem);
      

      // rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
      *((joystick_data_t*)tx_buffer) = server_stick_data;
      ESP_LOGI(TAG, "Received %d bytes: %s", sizeof(joystick_data_t), rx_buffer);
      int err = send(accept_sock, tx_buffer, sizeof(joystick_data_t), 0);
      if (err < 0)
      {
          ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
          break;
      }
      ESP_LOGI(TAG, "Sent %d bytes: %s", err, rx_buffer);
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }

  ESP_LOGI(TAG, "Closed");

  vTaskDelete(NULL);
}

static void client_circle_task(void *pvParameters)
{
  uint32_t pos_x = 100;
  uint32_t pos_y = 100;
  float speed = 4.0f;

  while (1)
  {
    if (xSemaphoreTake(client_sem, pdMS_TO_TICKS(20)) == pdTRUE)
    {
      float dir_x = client_stick_data.x;
      float dir_y = client_stick_data.y;

      ESP_LOGI(TAG, "circle dir: x -> %.2f, y -> %.2f", dir_x, dir_y);

      // xSemaphoreGive(client_sem);

      spi_display_draw_circle(device_handle, pos_x, pos_y, 40, 0xF800);
      pos_x += speed * dir_x;
      pos_y += speed * dir_y;
      vTaskDelay(100 / portTICK_PERIOD_MS);
      spi_display_draw_circle(device_handle, pos_x, pos_y, 40, 0xFEAF);
    }
    else
    {
      ESP_LOGI(TAG, "Unable take mutex, print skip");
    }
    // vTaskDelay(20 / portTICK_PERIOD_MS);

  }
}

void app_main()
{
  ESP_LOGI("main", "Main task stared");

  client_sem = xSemaphoreCreateBinary();
  server_sem = xSemaphoreCreateBinary();

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
  wifi_init_softap();

  int ret_ = joystick_controller_init();

  if (ret_ == 0) {
    ESP_LOGI("main", "Init joystick controller ok. Status code: %d", ret_);
  }
  else {
    ESP_LOGI("main", "Cannot init joystick controller. Status code: %d", ret_);
  }

  ret = 0;
  spi_device_handle_t spi_dev_h;

  spi_bus_config_t spi_cfg = {
      .mosi_io_num = CONFIG_PIN_NUM_MOSI,
      .miso_io_num = -1,
      .sclk_io_num = CONFIG_PIN_NUM_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 16*320*2+8,
      .flags = 0
  };

  spi_device_interface_config_t spi_dev_cfg = {
      .clock_speed_hz = 10*1000*1000,           //Clock out at 10 MHz
      .mode = 0,                                //SPI mode 0
      .spics_io_num = CONFIG_PIN_NUM_CS,               //CS pin
      .queue_size = 7,                          //We want to be able to queue 7 transactions at a time
      .pre_cb = &spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
  };

  ret = spi_bus_initialize(HSPI_HOST, &spi_cfg, SPI_DMA_CH_AUTO);
  ESP_LOGI(TAG, "spi bus initialize: %d", ret);

  ret = spi_bus_add_device(HSPI_HOST, &spi_dev_cfg, &spi_dev_h);
  ESP_LOGI(TAG, "spi bus add device: %d", ret);

  spi_display_init(spi_dev_h, DISPLAY_SIZE_X, DISPLAY_SIZE_Y);

  device_handle = spi_dev_h;

  xTaskCreatePinnedToCore(&tcp_server_task, "tcp_client", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(&client_circle_task, "ext_circle", 4096, NULL, 2, NULL, 0);

  spi_display_fill(spi_dev_h, 0xF800);

  uint32_t pos_x = DISPLAY_SIZE_Y / 2;
  uint32_t pos_y = DISPLAY_SIZE_X / 2;
  spi_display_draw_circle(spi_dev_h, pos_x, pos_y, 40, 0xFFFF);

  float speed = 4.0f;

  while (1)
  {
    joystick_data_get(&server_stick_data);
    uint32_t new_pos_x = pos_x + speed * server_stick_data.x;
    uint32_t new_pos_y = pos_y + speed * server_stick_data.y;

    if (new_pos_x == pos_x && new_pos_y == pos_y)
    {
      continue;
    }

    spi_display_draw_circle(spi_dev_h, pos_x, pos_y, 40, 0xF800);

    vTaskDelay(70 / portTICK_PERIOD_MS);

    pos_x = new_pos_x;
    pos_y = new_pos_y;

    spi_display_draw_circle(spi_dev_h, pos_x, pos_y, 40, 0xFFFF);
  }
  
}