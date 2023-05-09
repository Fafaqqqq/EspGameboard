#include <stdio.h>
#include <string.h>

#include "joystick.h"
// #include "spi_display.h"
#include "display_api.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_intr_alloc.h"

#include "esp_log.h"
// #include "esp_mac.h"
// #include "esp_wifi.h"
// #include "esp_event.h"

#include "nvs_flash.h"
#include "driver/gpio.h"

#include "lwip/ip6_addr.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_init.h"

// #define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
// #define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
// #define EXAMPLE_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
// #define EXAMPLE_MAX_STA_CONN       CONFIG_ESP_MAX_STA_CONN

#define HOST_IP_ADDR "192.168.4.1"
#define PORT 3333
#define BUTTON_GPIO_PIN     14  // номер GPIO-пина, к которому подключена кнопка
#define BUTTON_ACTIVE_LEVEL 0   // уровень логической активности кнопки

const char *TAG = "master_target";

static volatile SemaphoreHandle_t client_sem = NULL;
static volatile SemaphoreHandle_t server_sem = NULL;

static volatile SemaphoreHandle_t wifi_init_sem = NULL;

static volatile joystick_data_t client_stick_data;
static volatile joystick_data_t server_stick_data;

// static volatile spi_device_handle_t device_handle;


static void client_circle_task(void *pvParameters);
static void tcp_server_task(void *pvParameters);

static void IRAM_ATTR button_isr_handler(void* arg)  {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  xHigherPriorityTaskWoken = pdFALSE;

  /* Unblock the task by releasing the semaphore. */
  xSemaphoreGive( wifi_init_sem);

}

// void spi_pre_transfer_callback(spi_transaction_t* spi_trans)
// {
//   int dc = (int)spi_trans->user;
//   gpio_set_level(CONFIG_PIN_NUM_DC, dc);
// }

void tcp_server_task(void *pvParameters)
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

// void client_circle_task(void *pvParameters)
// {
//   uint32_t pos_x = 100;
//   uint32_t pos_y = 100;
//   float speed = 4.0f;

//   while (1)
//   {
//     if (xSemaphoreTake(client_sem, pdMS_TO_TICKS(20)) == pdTRUE)
//     {
//       float dir_x = client_stick_data.x;
//       float dir_y = client_stick_data.y;

//       ESP_LOGI(TAG, "circle dir: x -> %.2f, y -> %.2f", dir_x, dir_y);

//       // xSemaphoreGive(client_sem);

//       spi_display_draw_circle(device_handle, pos_x, pos_y, 40, 0xF800);
//       pos_x += speed * dir_x;
//       pos_y += speed * dir_y;
//       vTaskDelay(100 / portTICK_PERIOD_MS);
//       spi_display_draw_circle(device_handle, pos_x, pos_y, 40, 0xFEAF);
//     }
//     else
//     {
//       vTaskDelay(100 / portTICK_PERIOD_MS);
//       // ESP_LOGI(TAG, "Unable take mutex, print skip");
//     }
//     // vTaskDelay(20 / portTICK_PERIOD_MS);

//   }
// }

void app_main()
{
  ESP_LOGI("main", "Main task stared");

  // client_sem = xSemaphoreCreateBinary();
  // server_sem = xSemaphoreCreateBinary();

  wifi_init();

  joystick_controller_init();
  display_init();


  // if (ret == 0) {
  //   ESP_LOGI("main", "Init joystick controller ok. Status code: %d", ret);
  // }
  // else {
  //   ESP_LOGI("main", "Cannot init joystick controller. Status code: %d", ret);
  // }

  // spi_device_handle_t spi_dev_h;

  // spi_bus_config_t spi_cfg = {
  //     .mosi_io_num = CONFIG_PIN_NUM_MOSI,
  //     .miso_io_num = -1,
  //     .sclk_io_num = CONFIG_PIN_NUM_CLK,
  //     .quadwp_io_num = -1,
  //     .quadhd_io_num = -1,
  //     .max_transfer_sz = 16*320*2+8,
  //     .flags = 0
  // };

  // spi_device_interface_config_t spi_dev_cfg = {
  //     .clock_speed_hz = 10*1000*1000,           //Clock out at 10 MHz
  //     .mode = 0,                                //SPI mode 0
  //     .spics_io_num = CONFIG_PIN_NUM_CS,               //CS pin
  //     .queue_size = 7,                          //We want to be able to queue 7 transactions at a time
  //     .pre_cb = &spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
  // };

  // esp_err_t ret = spi_bus_initialize(HSPI_HOST, &spi_cfg, SPI_DMA_CH_AUTO);
  // ESP_LOGI(TAG, "spi bus initialize: %d", ret);

  // ret = spi_bus_add_device(HSPI_HOST, &spi_dev_cfg, &spi_dev_h);
  // ESP_LOGI(TAG, "spi bus add device: %d", ret);

  // spi_display_init(spi_dev_h, DISPLAY_SIZE_X, DISPLAY_SIZE_Y);

  // device_handle = spi_dev_h;

  // spi_display_fill(device_handle, 0xF800);
  spi_display_fill(0xF800);


  int32_t pos_x = 240 / 2;
  // uint32_t pos_y = DISPLAY_SIZE_X / 2;
  // spi_display_draw_circle(spi_dev_h, pos_x, pos_y, 40, 0xFFFF);

  float speed = 1.0f;
  // pixel_t centre;
  // centre.x = 50;
  // centre.y = 50;

  // display_send_buffer();
  // // vTaskDelay(pdMS_TO_TICKS(500));

  // display_fill_color(0xFFFF);
  // display_draw_circle(&centre, 30, 0x0000);
  // display_send_buffer();
  // spi_display_draw_plot(device_handle, pos_x, DISPLAY_SIZE_Y - 100, 0xFFFF);
  display_draw_plot(pos_x, 0xFFFF);

  while (1)
  {

    joystick_data_get(&server_stick_data);

    uint32_t new_pos_x = pos_x + speed * server_stick_data.x;
    // uint32_t new_pos_y = pos_y + speed * server_stick_data.y;

    if (new_pos_x == pos_x)
    {
      continue;
    }
    // spi_display_fill(0xF800);
    // display_fill_color(0xFFFF);
  //   display_send_buffer();
    // spi_display_draw_circle(spi_dev_h, pos_x, pos_y, 40, 0xF800);

    // centre.x += 1;
    // centre.y += 1;
    

    // display_fill_color(0xFFFF);
    // display_draw_circle(&centre, 30, 0x0000);
    // display_send_buffer();
    // spi_display_draw_plot(device_handle, pos_x, DISPLAY_SIZE_Y - 100, 0xF800);
    display_draw_plot(pos_x, 0xF800);

    vTaskDelay(20 / portTICK_PERIOD_MS);

    pos_x = new_pos_x;


    // pos_y = new_pos_y;

    // spi_display_draw_circle(spi_dev_h, pos_x, pos_y, 40, 0xFFFF);
      // spi_display_draw_plot(device_handle, pos_x, DISPLAY_SIZE_Y - 100, 0xFFFF);
    display_draw_plot(pos_x, 0xFFFF);
    
  }
  
}