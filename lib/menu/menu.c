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
  { "Pong", pong_game },
  { "Snake", NULL },
  { "Tanks", NULL },
};

static MenuItem_t rooms_menu_items[] = {
  { "Conntect to room.", wifi_init_sta },
  { "Create the room.", wifi_init_ap },
};

static void print_welcome()
{
  const char* welcome = "Welcome to the";
  const char* esp_gameboard = "ESP Gameboard";
  const char* gameboard = " gameboard";

  DisplayFill(TFT9341_BLACK);
  DisplaySetTextColor(TFT9341_MAGENTA);
  DisplaySetFont(&Font20);
  DisplaySetBackColor(TFT9341_BLACK);

  DisplayDrawString(welcome, TFT9341_WIDTH / 2 - strlen(welcome) / 2 * Font20.Width, TFT9341_HEIGHT / 2  - 1 * Font20.Height);

  DisplaySetFont(&Font24);
  DisplaySetTextColor(TFT9341_MAGENTA);

  // DisplaySetTextColor(TFT9341_BLUE);
  DisplayDrawSymbol(TFT9341_WIDTH / 2 - strlen(esp_gameboard) / 2 * Font24.Width, TFT9341_HEIGHT / 2 + Font24.Height, esp_gameboard[0]);
  
  DisplayDrawSymbol(TFT9341_WIDTH / 2 - (strlen(esp_gameboard) - 2) / 2 * Font24.Width, TFT9341_HEIGHT / 2 + Font24.Height, esp_gameboard[1]);
  
  // DisplaySetTextColor(TFT9341_YELLOW);
  DisplayDrawSymbol(TFT9341_WIDTH / 2 - (strlen(esp_gameboard) - 4) / 2 * Font24.Width, TFT9341_HEIGHT / 2 + Font24.Height, esp_gameboard[2]);

  // DisplaySetTextColor(TFT9341_MAGENTA);
  DisplayDrawString(esp_gameboard + 3, TFT9341_WIDTH / 2 - (strlen(welcome) - 8) / 2 * Font24.Width, TFT9341_HEIGHT / 2 + Font24.Height);


  vTaskDelay(pdMS_TO_TICKS(10000));
}

static void StartRoomsMenu(MenuItemFunc_t exec_game)
{

  DisplayFill(TFT9341_BLACK);
  DisplaySetTextColor(TFT9341_WHITE);
  DisplaySetBackColor(TFT9341_BLACK);
  DisplaySetFont(&Font20);
  int current = 0;

  for (uint32_t i = 0; i < 2; i++)
  {

    DisplayDrawSymbol(TFT9341_WIDTH / 2 - 70 - 3 * 17, TFT9341_HEIGHT / 2 - 20 + i * 24, i + 1 + '0');
    DisplayDrawSymbol(TFT9341_WIDTH / 2 - 70 - 2 * 17, TFT9341_HEIGHT / 2 - 20 + i * 24, '.');

    DisplayDrawString(rooms_menu_items[i].name, TFT9341_WIDTH / 2 - 100 + 17, TFT9341_HEIGHT / 2 - 20 + i * 24);
  }

  joystick_data_t joystick_data;
  
  DisplayDrawSymbol(TFT9341_WIDTH / 2 - 70 - 4 * 17, TFT9341_HEIGHT / 2 - 20 + current * 24, '>');

  while (1)
  {
    joystick_data_get(&joystick_data);
    
    if (joystick_data.is_pressed == 1)
    {
      // menu_items[current].func(NULL);
      // DisplayFill(TFT9341_BLACK);
      // DisplayDrawString("Wait for connect...", TFT9341_WIDTH / 2 - strlen("Wait connect...") / 2 * Font20.Width - 2 * Font20.Width, TFT9341_HEIGHT / 2);
      
      // rooms_menu_items[current].func("espgameroom");
      // wifi_init_ap("espgameroom", "espgameroom");
      exec_game((void*)current);
      break;
    }

    if (fabs(joystick_data.y) == 1.0)
    {
      DisplayDrawSymbol(TFT9341_WIDTH / 2 - 70 - 4 * 17, TFT9341_HEIGHT / 2 - 20 + current * 24, ' ');

      current += joystick_data.y; 
      if (current == 2)
      {
        current = 0;
      }
      else if (current == -1)
      {
        current = 1;
      }

      DisplayDrawSymbol(TFT9341_WIDTH / 2 - 70 - 4 * 17, TFT9341_HEIGHT / 2 - 20 + current * 24, '>');
    }


    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void MenuInit()
{
  print_welcome();


  DisplayFill(TFT9341_BLACK);
  DisplaySetTextColor(TFT9341_WHITE);
  DisplaySetBackColor(TFT9341_BLACK);
  DisplaySetFont(&Font20);
  
  for (uint32_t i = 0; i < 3; i++)
  {

    DisplayDrawSymbol(TFT9341_WIDTH / 2 - 10 - 3 * 17, TFT9341_HEIGHT / 2 - 20 + i * 24, i + 1 + '0');
    DisplayDrawSymbol(TFT9341_WIDTH / 2 - 10 - 2 * 17, TFT9341_HEIGHT / 2 - 20 + i * 24, '.');

    DisplayDrawString(menu_items[i].name, TFT9341_WIDTH / 2 - 30 + 17, TFT9341_HEIGHT / 2 - 20 + i * 24);
  }

  DisplayDrawSymbol(TFT9341_WIDTH / 2 - 10 - 4 * 17, TFT9341_HEIGHT / 2 - 20, '>');
}

void StartMenu()
{
  MenuInit();

  int current = 0;
  joystick_data_t joystick_data;
  
  while (1)
  {
    joystick_data_get(&joystick_data);

    if (joystick_data.is_pressed == 1)
    {
      // menu_items[current].func(NULL);
      StartRoomsMenu(menu_items[current].func);
      MenuInit();
      continue;
    }

    if (fabs(joystick_data.y) == 1.0)
    {
      DisplayDrawSymbol(TFT9341_WIDTH / 2 - 10 - 4 * 17, TFT9341_HEIGHT / 2 - 20 + current * 24, ' ');

      current += joystick_data.y; 
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