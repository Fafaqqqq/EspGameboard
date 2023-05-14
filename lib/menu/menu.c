#include "menu.h"
#include "pong.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include <driver/gpio.h>

#include "esp_err.h"
#include "esp_log.h"

#include "sdkconfig.h"

#include "display_api.h"
#include "joystick.h"
#include "wifi.h"

#include <math.h>

typedef int MenuItemFunc_t(void*);
typedef const char* MenuItemName_t;

typedef struct
{
  MenuItemName_t name;
  MenuItemFunc_t *func;
} MenuItem_t;

static MenuItem_t menu_items[] = {
  { "Pong", NULL },
  { "Snake", NULL },
  { "Tanks", NULL },
};

void StartMenu()
{
  DisplayFill(TFT9341_BLACK);
  DisplaySetTextColor(TFT9341_WHITE);
  DisplaySetBackColor(TFT9341_BLACK);
  DisplaySetFont(&Font20);
  int current = 0;
  
  for (uint32_t i = 0; i < 3; i++)
  {
    // if (i == current) 
    // {
    //   DisplayDrawSymbol(TFT9341_WIDTH / 2 - 10 - 4 * 17, TFT9341_HEIGHT / 2 - 20 + i * 24, '>');
    // }
    DisplayDrawSymbol(TFT9341_WIDTH / 2 - 10 - 3 * 17, TFT9341_HEIGHT / 2 - 20 + i * 24, i + 1 + '0');
    DisplayDrawSymbol(TFT9341_WIDTH / 2 - 10 - 2 * 17, TFT9341_HEIGHT / 2 - 20 + i * 24, '.');

    DisplayDrawString(menu_items[i].name, TFT9341_WIDTH / 2 - 30 + 17, TFT9341_HEIGHT / 2 - 20 + i * 24);
  }

  joystick_data_t joystick_pos;
  
  DisplayDrawSymbol(TFT9341_WIDTH / 2 - 10 - 4 * 17, TFT9341_HEIGHT / 2 - 20 + current * 24, '>');


  while (1)
  {
    joystick_data_get(&joystick_pos);

    // ESP_LOGI("Menu", "WORK");
    if (fabs(joystick_pos.y) == 1.0)
    {
      DisplayDrawSymbol(TFT9341_WIDTH / 2 - 10 - 4 * 17, TFT9341_HEIGHT / 2 - 20 + current * 24, ' ');

      current += joystick_pos.y; 
      if (current == 3)
      {
        current = 0;
      }
      else if (current == -1)
      {
        current = 2;
      }

      DisplayDrawSymbol(TFT9341_WIDTH / 2 - 10 - 4 * 17, TFT9341_HEIGHT / 2 - 20 + current * 24, '>');
    }


    vTaskDelay(pdMS_TO_TICKS(100));
  }
}