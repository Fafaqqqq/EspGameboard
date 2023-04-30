#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "joystick.h"

#include <stdio.h>
#include "esp_log.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "math.h"

#define MIN_VALUE 0.1
#define MAX_VALUE 0.60
#define SIGN(X) (sign(X))

#define JOYSTICK_X_CHANNEL ADC1_CHANNEL_6 // задаем канал для измерения оси X джойстика
#define JOYSTICK_Y_CHANNEL ADC1_CHANNEL_7 // задаем канал для измерения оси Y джойстика
#define JOYSTICK_X_GPIO GPIO_NUM_34 // GPIO-номер, соответствующий оси X джойстика
#define JOYSTICK_Y_GPIO GPIO_NUM_35 // GPIO-номер, соответствующий оси Y джойстика

static int sign(float x)
{
  if (x > 0) return 1;
  if (x < 0) return -1;

  return 0;
}

static const char* TAG = "joystick"; // задаем тег для вывода отладочных сообщений

static joystick_data_t _joystick_data;
static joystick_data_t _joystick_center;

joystick_status_t joystick_data_get(joystick_data_t* joystick_data) {
  uint32_t joystick_x_val = adc1_get_raw(JOYSTICK_X_CHANNEL); // считываем значение оси X
  uint32_t joystick_y_val = adc1_get_raw(JOYSTICK_Y_CHANNEL); // считываем значение оси Y
  
  joystick_data->x = roundf(((float)joystick_x_val / 2048.0f - (float)_joystick_center.x / 2048.0f) * 100.0f) / 100.0f;
  joystick_data->y = roundf(((float)joystick_y_val / 2048.0f - (float)_joystick_center.y / 2048.0f) * 100.0f) / 100.0f;
  
  if (fabs(joystick_data->x) < MIN_VALUE) joystick_data->x = 0;
  if (fabs(joystick_data->x) > MAX_VALUE) joystick_data->x = SIGN(joystick_data->x);

  if (fabs(joystick_data->y) < MIN_VALUE) joystick_data->y = 0;
  if (fabs(joystick_data->y) > MAX_VALUE) joystick_data->y = SIGN(joystick_data->y);

  return joystick_ok;
}

joystick_status_t joystick_controller_init() {
  adc1_config_width(ADC_WIDTH_BIT_12); // задаем разрядность АЦП
  adc1_config_channel_atten(JOYSTICK_X_CHANNEL, ADC_ATTEN_DB_11); // задаем усиление для канала оси X
  adc1_config_channel_atten(JOYSTICK_Y_CHANNEL, ADC_ATTEN_DB_11); // задаем усиление для канала оси Y

  esp_rom_gpio_pad_select_gpio(JOYSTICK_X_GPIO); // настраиваем GPIO для оси X
  esp_rom_gpio_pad_select_gpio(JOYSTICK_Y_GPIO); // настраиваем GPIO для оси Y

  _joystick_center.x = adc1_get_raw(JOYSTICK_X_CHANNEL); // получаем значение оси X в центре
  _joystick_center.y = adc1_get_raw(JOYSTICK_Y_CHANNEL); // получаем значение оси Y в центре
  
  return joystick_ok;
}