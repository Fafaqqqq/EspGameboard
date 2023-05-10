// #include <stdio.h>
// #include <string.h>

#include "joystick.h"
#include "graphics.h"
#include "display_api.h"
#include "wifi_init.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// #include "esp_log.h"

// const char *TAG = "master_target";

void app_main()
{
  // ESP_LOGI(TAG, "Main task stared");

  wifi_init();
  joystick_controller_init();
  graphics_init();
  // graphics_fill_disp(0x07E0);
  spi_fill(0x07E0);

  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

}