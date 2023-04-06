#include <stdio.h>

#include "joystick.h"
#include "spi_display.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <sdkconfig.h>

extern void spi_display_init(spi_device_handle_t spi, uint16_t w_size, uint16_t h_size);

void spi_pre_transfer_caklback(spi_transaction_t* spi_trans)
{
  int dc = (int)spi_trans->user;
  gpio_set_level(CONFIG_PIN_NUM_DC, dc);
}

const char *TAG = "main";

void app_main() {

  ESP_LOGI("main", "Main task stared");

  esp_err_t ret = 0;
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
      .pre_cb = &spi_pre_transfer_caklback,  //Specify pre-transfer callback to handle D/C line
  };

  ret = spi_bus_initialize(HSPI_HOST, &spi_cfg, SPI_DMA_CH_AUTO);
  ESP_LOGI(TAG, "spi bus initialize: %d", ret);

  ret = spi_bus_add_device(HSPI_HOST, &spi_dev_cfg, &spi_dev_h);
  ESP_LOGI(TAG, "spi bus add device: %d", ret);

  spi_display_init(spi_dev_h, DISPLAY_SIZE_X, DISPLAY_SIZE_Y);


  spi_display_fill(spi_dev_h, 0xF800);

  uint64_t frame_cnt = 0;

  uint32_t pos_x = DISPLAY_SIZE_X / 2;
  uint32_t pos_y = DISPLAY_SIZE_Y / 2;
  

  while (1) {
    // spi_display_fill(spi_dev_h, 0xF800);
    // spi_display_fill(spi_dev_h, 0x0000);
    // spi_display_fill(spi_dev_h, 0xF800);

    spi_display_draw_circle(spi_dev_h, pos_x, pos_y, 40, 0xF800);
    vTaskDelay(20 / portTICK_PERIOD_MS);

    if (frame_cnt++ % 5 == 0) {
      pos_x++;
      pos_y--;
    }
    spi_display_draw_circle(spi_dev_h, pos_x, pos_y, 40, 0xFFFF);

    if (frame_cnt == UINT64_MAX) frame_cnt = 0;
  }

  // joystick_handle_t h_joystick;
  // int ret = joystick_controller_init(&h_joystick);

  // if (ret == 0) {
  //   ESP_LOGI("main", "Init joystick controller ok. Status code: %d", ret);
  // }
  // else {
  //   ESP_LOGI("main", "Cannot init joystick controller. Status code: %d", ret);
  // }


  // while (1)
  // {
  //   if (ret == 0) {
  //     joystick_data_t data;

  //     joystick_data_get(&data);

  //     vTaskDelay(pdMS_TO_TICKS(200));

  //     ESP_LOGI("joystick data", "Axis data x: %f, y: %f", data.x, data.y);
  //   }
  // }
  
}