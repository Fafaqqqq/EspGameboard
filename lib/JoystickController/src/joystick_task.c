#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "joystick.h"

#include <stdio.h>
#include "esp_log.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "math.h"

#define JOYSTICK_X_CHANNEL ADC1_CHANNEL_6 // задаем канал для измерения оси X джойстика
#define JOYSTICK_Y_CHANNEL ADC1_CHANNEL_7 // задаем канал для измерения оси Y джойстика
#define JOYSTICK_X_GPIO GPIO_NUM_34 // GPIO-номер, соответствующий оси X джойстика
#define JOYSTICK_Y_GPIO GPIO_NUM_35 // GPIO-номер, соответствующий оси Y джойстика

static const char* TAG = "joystick"; // задаем тег для вывода отладочных сообщений

static joystick_data_t _joystick_data;
static joystick_data_t _joystick_center;

static SemaphoreHandle_t h_joystick_mtx = NULL;
static TaskHandle_t h_joystick_task = NULL;

void joystick_read_task(void* params) {

  while (1) {
    xSemaphoreTake(h_joystick_mtx, portMAX_DELAY);
    // ESP_LOGE(TAG, "joystick task started");
    uint32_t joystick_x_val = adc1_get_raw(JOYSTICK_X_CHANNEL); // считываем значение оси X
    uint32_t joystick_y_val = adc1_get_raw(JOYSTICK_Y_CHANNEL); // считываем значение оси Y

    // ESP_LOGE(TAG, "joystick task ended");
    // вычитаем среднее значение для каждой оси, чтобы в центре были значения 0.0
    _joystick_data.x = roundf(((float)joystick_x_val / 2048.0f - (float)_joystick_center.x / 2048.0f) * 100.0f) / 100.0f;
    _joystick_data.y = roundf(((float)joystick_y_val / 2048.0f - (float)_joystick_center.y / 2048.0f) * 100.0f) / 100.0f;

    // ESP_LOGI(TAG, "x: %.2f, y: %.2f\n", _joystick_data.x, _joystick_data.y);
    // vTaskDelay(pdMS_TO_TICKS(100)); // задержка между чтением данных джойстика
    // ESP_LOGI(TAG, "x: %.2f, y: %.2f\n", _joystick_data.x, _joystick_data.y);

    xSemaphoreGive(h_joystick_mtx);
  }

  vTaskDelete(h_joystick_task);
}

joystick_status_t joystick_data_get(joystick_data_t* joystick_data) {
  if (h_joystick_mtx) {
    xSemaphoreTake(h_joystick_mtx, portMAX_DELAY);
    joystick_data->x = _joystick_data.x;
    joystick_data->x = _joystick_data.y;
    xSemaphoreGive(h_joystick_mtx);
  }
  else {
    return joystick_bad_mutex;
  }

  return joystick_ok;
}

joystick_status_t joystick_controller_init(joystick_handle_t* joystick_handle) {
  if (h_joystick_mtx == NULL) {
      h_joystick_mtx = xSemaphoreCreateMutex();
      if (h_joystick_mtx == NULL) {
          ESP_LOGE(TAG, "Failed to create h_joystick_mtx");
          return joystick_bad_mutex;
      }
  }

  adc1_config_width(ADC_WIDTH_BIT_12); // задаем разрядность АЦП
  adc1_config_channel_atten(JOYSTICK_X_CHANNEL, ADC_ATTEN_DB_11); // задаем усиление для канала оси X
  adc1_config_channel_atten(JOYSTICK_Y_CHANNEL, ADC_ATTEN_DB_11); // задаем усиление для канала оси Y

  esp_rom_gpio_pad_select_gpio(JOYSTICK_X_GPIO); // настраиваем GPIO для оси X
  esp_rom_gpio_pad_select_gpio(JOYSTICK_Y_GPIO); // настраиваем GPIO для оси Y

  _joystick_center.x = adc1_get_raw(JOYSTICK_X_CHANNEL); // получаем значение оси X в центре
  _joystick_center.y = adc1_get_raw(JOYSTICK_Y_CHANNEL); // получаем значение оси Y в центре

  
  // xTaskCreatePinnedToCore(joystick_read_task, "Joystick Task", 2048, NULL, 1, NULL, 0);

  if (xTaskCreatePinnedToCore(joystick_read_task, "joystick_task", 16384, NULL, 2, &h_joystick_task, 1) != pdPASS) {
    return joystick_bad_task;
  }

  return joystick_ok;
}