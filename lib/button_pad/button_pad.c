#include "button_pad.h"

#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define BUTTON_PIN_RED  GPIO_NUM_13
#define BUTTON_PIN_BLUE GPIO_NUM_14

static button_intr_func   red_intr_func_  = NULL;
static button_intr_params red_intr_params_ = NULL;

static button_intr_func   blue_intr_func_  = NULL;
static button_intr_params blue_intr_params_ = NULL;

static SemaphoreHandle_t sem_register_red_ = NULL;
static SemaphoreHandle_t sem_register_blue_ = NULL;

static const char* TAG = "button_pad";

static void button_task_intr
(
  void* arg
)
{
  uint32_t button_type = *(uint32_t*)arg;

  while (1) {
    while (gpio_get_level(button_type))
    {
      vTaskDelay(pdMS_TO_TICKS(20));
    }

    while (!gpio_get_level(button_type))
    {
      vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (xSemaphoreTake(button_type ? sem_register_red_ : sem_register_blue_, pdMS_TO_TICKS(100)) == pdFALSE)
    {
      continue;
    }

    // ESP_LOGI(TAG, "pressed pin %lu", button_type);

    if (button_type == BUTTON_PIN_RED &&
        red_intr_func_)
    {
      red_intr_func_(red_intr_params_);
    }
    else if (red_intr_func_)
    {
      blue_intr_func_(blue_intr_params_);
    }

    xSemaphoreGive(button_type ? sem_register_red_ : sem_register_blue_);

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void button_pad_init()
{
  sem_register_red_ = xSemaphoreCreateMutex();
  sem_register_blue_ = xSemaphoreCreateMutex();

  esp_rom_gpio_pad_select_gpio(BUTTON_PIN_RED);
  gpio_set_direction(BUTTON_PIN_RED, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BUTTON_PIN_RED, GPIO_PULLUP_ONLY);

  esp_rom_gpio_pad_select_gpio(BUTTON_PIN_BLUE);
  gpio_set_direction(BUTTON_PIN_BLUE, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BUTTON_PIN_BLUE, GPIO_PULLUP_ONLY);

  uint32_t button_type_red  = BUTTON_PIN_RED;
  uint32_t button_type_blue = BUTTON_PIN_BLUE;

  xTaskCreatePinnedToCore(button_task_intr, "button_task_intr_red", 4096, &button_type_red, 9, NULL, 1);
  xTaskCreatePinnedToCore(button_task_intr, "button_task_intr_blue", 4096, &button_type_blue, 10, NULL, 1);
}

int button_register_intr
(
  button_type_t btn_type,
  button_intr_func intr_func,
  button_intr_params intr_params
)
{
  if (!intr_func)
  {
    return -__LINE__;
  }

  if (xSemaphoreTake(btn_type ? sem_register_red_ : sem_register_blue_, pdMS_TO_TICKS(100)) == pdFALSE)
  {
    return -__LINE__;
  }

  
  if ((btn_type && (red_intr_func_  || red_intr_params_)) ||
     (!btn_type && (blue_intr_func_ || blue_intr_params_)))
  {
    return -__LINE__;
  }

  
  if (btn_type)
  {
    red_intr_func_   = intr_func;
    red_intr_params_ = intr_params;
  }
  else
  {
    blue_intr_func_   = intr_func;
    blue_intr_params_ = intr_params;
  }

  xSemaphoreGive(btn_type ? sem_register_red_ : sem_register_blue_);

  return 0;
}

int button_unregister_intr
(
  button_type_t btn_type
)
{
  if (xSemaphoreTake(btn_type ? sem_register_red_ : sem_register_blue_, pdMS_TO_TICKS(100)) == pdFALSE)
  {
    return -__LINE__;
  }

  if ((btn_type && (!red_intr_func_  || !red_intr_params_)) ||
     (!btn_type && (!blue_intr_func_ || !blue_intr_params_)))
  {
    return -__LINE__;
  }

  if (btn_type)
  {
    red_intr_func_   = NULL;
    red_intr_params_ = NULL;
  }
  else
  {
    blue_intr_func_   = NULL;
    blue_intr_params_ = NULL;
  }

  xSemaphoreGive(btn_type ? sem_register_red_ : sem_register_blue_);

  return 0;
}