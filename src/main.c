#include <stdio.h>
#include "joystick.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// void vTaskFunction( void *pvParameters )
// {
//   while (1)
//   {
//     ESP_LOGD("TASK", "tusk running");
//     // printf("task running");
//     vTaskDelay(1000 / portTICK_PERIOD_MS);
//   }
  
// }

void app_main() {

  // xTaskCreate( vTaskFunction, "TaskName", configMINIMAL_STACK_SIZE, NULL, 1, NULL );

  // vTaskStartScheduler();

  // while (1)
  // {
  //   ESP_LOGI("MAIN", "working");
  //   vTaskDelay(pdMS_TO_TICKS(1000));
  // }
  
  ESP_LOGI("main", "Main task stared");

  joystick_handle_t h_joystick;
  int ret = joystick_controller_init(&h_joystick);

  if (ret == 0) {
    ESP_LOGI("main", "Init joystick controller ok. Status code: %d", ret);

    vTaskStartScheduler();
  }
  else {
    ESP_LOGI("main", "Cannot init joystick controller. Status code: %d", ret);
  }


    // while (1)
    // {
    //   // if (ret == 0) {
    //   //   joystick_data_t data;

    //   // // ESP_LOGI("main", "work");
    //   //   joystick_data_get(&data);
    //   // // ESP_LOGI("main", "work1");

    //   //   vTaskDelay(pdMS_TO_TICKS(100));

    //   //   ESP_LOGI("joystick data", "Axis data x: %.2f, y: %.2f", data.x, data.y);
    //   // }
    //   // else {
    //   //   // ESP_LOGI
    //   // }
    // }
  
}