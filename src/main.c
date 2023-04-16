#include <stdio.h>

#include "joystick.h"
#include "display_api.h"

#include "esp_log.h"
#include "esp_task_wdt.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sdkconfig.h>

// extern void spi_display_init(spi_device_handle_t spi, uint16_t w_size, uint16_t h_size);

// void spi_pre_transfer_callback(spi_transaction_t* spi_trans)
// {
//   int dc = (int)spi_trans->user;
//   gpio_set_level(CONFIG_PIN_NUM_DC, dc);
// }

const char *TAG = "main";

void print_task(void*)
{
  while (1)
  {
    ESP_LOGI(TAG, "print task");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void app_main() {
  ESP_LOGI("main", "Main task stared");

  TaskHandle_t print_task_h = NULL; 

  xTaskCreate(&print_task, "print_task", 4096, NULL, 2, &print_task_h);
  if (NULL == print_task_h)
  {
    ESP_LOGE(TAG, "Cant create printf");
  }

  joystick_data_t data;
  int ret_ = joystick_controller_init();

  if (ret_ == 0) {
    ESP_LOGI("main", "Init joystick controller ok. Status code: %d", ret_);
  }
  else {
    ESP_LOGI("main", "Cannot init joystick controller. Status code: %d", ret_);
  }

  esp_err_t ret = 0;
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

  // ret = spi_bus_initialize(HSPI_HOST, &spi_cfg, SPI_DMA_CH_AUTO);
  // ESP_LOGI(TAG, "spi bus initialize: %d", ret);

  // ret = spi_bus_add_device(HSPI_HOST, &spi_dev_cfg, &spi_dev_h);
  // ESP_LOGI(TAG, "spi bus add device: %d", ret);

  // spi_display_init(spi_dev_h, DISPLAY_SIZE_X, DISPLAY_SIZE_Y);


  display_init();
  display_fill_color(0xF800);

  uint64_t frame_cnt = 0;

  // uint32_t pos_x = DISPLAY_SIZE_Y / 2;
  // uint32_t pos_y = DISPLAY_SIZE_X / 2;
  
  float speed = 4.0f;

  while (1) {

    // spi_display_draw_circle(spi_dev_h, pos_x, pos_y, 40, 0xF800);
    joystick_data_get(&data);

    vTaskDelay(20 / portTICK_PERIOD_MS);

    // pos_x += speed * data.x;
    // pos_y += speed * data.y;

    // spi_display_draw_circle(spi_dev_h, pos_x, pos_y, 40, 0xFFFF);

    if (frame_cnt == UINT64_MAX) frame_cnt = 0;
  }
  
}