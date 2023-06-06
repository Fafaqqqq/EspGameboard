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
#include "button_pad.h"

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

static void print_buttons()
{
  uint16_t circle_radius = 20;
  uint16_t blue_circle_x = TFT9341_WIDTH / 2 - 90, blue_circle_y = TFT9341_HEIGHT - 50;
  uint16_t red_circle_x  = TFT9341_WIDTH / 2 + 40, red_circle_y  = TFT9341_HEIGHT - 50;

  uint16_t accept_start_x = blue_circle_x + circle_radius + 40;
  uint16_t refuse_start_x = red_circle_x  + circle_radius + 40;

  display_fill_circle(circle_radius, blue_circle_x, blue_circle_y, TFT9341_BLUE);
  display_fill_circle(circle_radius, red_circle_x, red_circle_y, TFT9341_RED);

  display_draw_line(TFT9341_WHITE, blue_circle_x + circle_radius + 10, blue_circle_y,
                                   blue_circle_x + circle_radius + 30, blue_circle_y);

  display_draw_line(TFT9341_WHITE, red_circle_x + circle_radius + 10, red_circle_y,
                                   red_circle_x + circle_radius + 30, red_circle_y);

  display_draw_line(TFT9341_GREEN, accept_start_x,     blue_circle_y - 5, accept_start_x + 5, blue_circle_y + 10);
  display_draw_line(TFT9341_GREEN, accept_start_x + 5, blue_circle_y + 10, accept_start_x + 10, blue_circle_y - 15);

  display_draw_line(TFT9341_RED, refuse_start_x, red_circle_y - 15, refuse_start_x + 20, red_circle_y + 15);
  display_draw_line(TFT9341_RED, refuse_start_x, red_circle_y + 15, refuse_start_x + 20, red_circle_y - 15);
}

static void print_welcome()
{
  const char* esp_welcome   = "Welcome to the";
  const char* esp_gameboard = "ESP Gameboard";

  display_fill(TFT9341_BLACK);
  display_set_text_color(TFT9341_MAGENTA);
  display_set_font(&Font20);
  display_set_back_color(TFT9341_BLACK);

  display_draw_string(esp_welcome,
                    TFT9341_WIDTH / 2 - strlen(esp_welcome) / 2 * Font20.Width,
                    TFT9341_HEIGHT / 2  - 1 * Font20.Height);

  display_set_font(&Font24);

  for (uint32_t i = 0; i < 3; i++)
  {
    display_draw_symbol(TFT9341_WIDTH / 2 - (strlen(esp_gameboard) - i * 2) / 2 * Font24.Width,
                      TFT9341_HEIGHT / 2 + Font24.Height,
                      esp_gameboard[i]);
  }

  display_draw_string(esp_gameboard + 3, 
                    TFT9341_WIDTH / 2 - (strlen(esp_welcome) - 8) / 2 * Font24.Width,
                    TFT9341_HEIGHT / 2 + Font24.Height);

  vTaskDelay(pdMS_TO_TICKS(5000));
}

static void print_coming_soon()
{
  display_fill(TFT9341_BLACK);

  print_buttons();
  display_set_text_color(TFT9341_WHITE);
  display_set_font(&Font20);
  display_set_back_color(TFT9341_BLACK);

  display_draw_string("Comming soon...", 
                      TFT9341_WIDTH / 2 - strlen("Comming soon...") / 2 * Font20.Width,
                      TFT9341_HEIGHT / 2);

  while (1)
  {
    if (button_pressed(BUTTON_RED) || button_pressed(BUTTON_BLUE))
    {
      return;
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}


static void start_rooms_menu(MenuItemFunc_t exec_game)
{
  button_flush(BUTTON_RED);
  button_flush(BUTTON_BLUE);

  int             current = 0;
  const char*     wait_connect = "Wait for connect...";
  joystick_data_t joystick_data = {0};

  display_fill(TFT9341_BLACK);
  display_set_text_color(TFT9341_WHITE);
  display_set_back_color(TFT9341_BLACK);
  display_set_font(&Font20);

  print_buttons();

  for (uint32_t i = 0; i < 2; i++)
  {
    display_draw_symbol(TFT9341_WIDTH  / 2 - 85 - 3 * Font20.Width,
                        TFT9341_HEIGHT / 2 - 20 + i * Font20.Height,
                        i + 1 + '0');

    display_draw_symbol(TFT9341_WIDTH  / 2 - 85 - 2 * Font20.Width,
                        TFT9341_HEIGHT / 2 - 20 + i * Font20.Height,
                        '.');

    display_draw_string(rooms_menu_items[i].name,
                        TFT9341_WIDTH  / 2 - 115 + 2 * Font20.Width,
                        TFT9341_HEIGHT / 2 - 20  + i * Font20.Height);
  }

  
  display_draw_symbol(TFT9341_WIDTH / 2 - 85 - 4 * Font20.Width, TFT9341_HEIGHT / 2 - 20 + current * Font20.Height, '>');

  while (1)
  {
    joystick_data_get(&joystick_data);
    
    if (button_pressed(BUTTON_BLUE))
    {
      display_fill(TFT9341_BLACK);
      display_draw_string(wait_connect,
                          TFT9341_WIDTH / 2 - strlen(wait_connect) / 2 * Font20.Width,
                          TFT9341_HEIGHT / 2);
    
      rooms_menu_items[current].func("espgameroom");
      
      exec_game((void*)current);
      break;
    }
    else if (button_pressed(BUTTON_RED))
    {
      break;
    }

    if (fabs(joystick_data.y) == 1.0)
    {
      display_draw_symbol(TFT9341_WIDTH  / 2 - 85 - 4 * Font20.Width,
                          TFT9341_HEIGHT / 2 - 20 + current * Font20.Height,
                          ' ');

      current += joystick_data.y; 
      if (current == 2)
      {
        current = 0;
      }
      else if (current == -1)
      {
        current = 1;
      }

      display_draw_symbol(TFT9341_WIDTH / 2 - 85 - 4 * Font20.Width,
                          TFT9341_HEIGHT / 2 - 20 + current * Font20.Height,
                          '>');
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void print_menu()
{
  display_fill(TFT9341_BLACK);
  display_set_text_color(TFT9341_WHITE);
  display_set_back_color(TFT9341_BLACK);
  display_set_font(&Font20);

  print_buttons();
  
  for (uint32_t i = 0; i < 3; i++)
  {

    display_draw_symbol(TFT9341_WIDTH / 2 - 10 - 3 * Font20.Width,
                        TFT9341_HEIGHT / 2 - 50 + i * Font20.Height,
                        i + 1 + '0');

    display_draw_symbol(TFT9341_WIDTH / 2 - 10 - 2 * Font20.Width,
                        TFT9341_HEIGHT / 2 - 50 + i * Font20.Height,
                        '.');

    display_draw_string(menu_items[i].name,
                        TFT9341_WIDTH / 2 - 30 + Font20.Width,
                        TFT9341_HEIGHT / 2 - 50 + i * Font20.Height);
  }

  display_draw_symbol(TFT9341_WIDTH / 2 - 15 - 4 * Font20.Width,
                      TFT9341_HEIGHT / 2 - 50,
                      '>');


}

void start_menu()
{
  button_flush(BUTTON_RED);
  button_flush(BUTTON_BLUE);
  
  print_welcome();
  print_menu();

  int current = 0;
  joystick_data_t joystick_data;
  
  while (1)
  {
    joystick_data_get(&joystick_data);

    if (button_pressed(BUTTON_BLUE))
    {

      if (current != 0)
      {
        print_coming_soon();
      }
      else
      {
        start_rooms_menu(menu_items[current].func);
      }

      print_menu();
      continue;
    }

    if (fabs(joystick_data.y) == 1.0)
    {
      display_draw_symbol(TFT9341_WIDTH / 2 - 15 - 4 * Font20.Width, TFT9341_HEIGHT / 2 - 50 + current * Font20.Height, ' ');

      current += joystick_data.y; 
      if (current == 3)
      {
        current = 0;
      }
      else if (current == -1)
      {
        current = 2;
      }

      display_draw_symbol(TFT9341_WIDTH / 2 - 15 - 4 * Font20.Width, TFT9341_HEIGHT / 2 - 50 + current * Font20.Height, '>');
    }


    vTaskDelay(pdMS_TO_TICKS(100));
  }
}