#include "games.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "display_api.h"
#include "joystick.h"
#include "wifi.h"
#include "button_pad.h"

#include <math.h>

typedef struct
{
  uint32_t x;
  uint32_t y;
} position_t;

typedef enum
{
  UP,
  DOWN,
  LEFT,
  RIGHT
} tank_direction;

typedef struct
{
  position_t pos;
  uint32_t   size_x;
  uint32_t   size_y;
  uint32_t   speed;
  tank_direction dir;
} tank_t;

static inline void swap(uint32_t* a, uint32_t *b)
{
  uint32_t temp = *a;
  *a = *b;
  *b = temp;
}

static inline void draw_tank(const tank_t* tank, uint16_t color)
{
  if (!tank->size_x || !tank->size_y)
    return;

  display_fill_rect(tank->pos.x, tank->pos.y,
                    tank->pos.x + tank->size_x, tank->pos.y + tank->size_y,
                    color);
  switch (tank->dir)
  {
    case UP:
      display_draw_line(color, tank->pos.x + tank->size_x / 2, tank->pos.y,
                               tank->pos.x + tank->size_x / 2, tank->pos.y - 10);
      break;
    case DOWN:
      display_draw_line(color, tank->pos.x + tank->size_x / 2, tank->pos.y + tank->size_y,
                               tank->pos.x + tank->size_x / 2, tank->pos.y + tank->size_y + 10);
      break;
    case LEFT:
      display_draw_line(color, tank->pos.x,     tank->pos.y + tank->size_y / 2,
                               tank->pos.x - 10, tank->pos.y + tank->size_y / 2);
      break;
    case RIGHT:
      display_draw_line(color, tank->pos.x + tank->size_x,     tank->pos.y + tank->size_y / 2,
                               tank->pos.x + tank->size_x + 10, tank->pos.y + tank->size_y / 2);
    default:
      break;
  }
}

static void get_new_position(tank_t* tank)
{
  joystick_data_t joystick_direction = {0};

  joystick_data_get(&joystick_direction);

  if (fabs(joystick_direction.x) == 1.0)
    {
      if (tank->dir == UP || tank->dir == DOWN)
      {
        swap(&tank->size_x, &tank->size_y);
      }

      if (joystick_direction.x > 0)
      {
        tank->dir = RIGHT;
      }
      if (joystick_direction.x < 0)
      {
        tank->dir = LEFT;
      }

      tank->pos.x += tank->speed * joystick_direction.x;
    }
    else if (fabs(joystick_direction.y) == 1.0)
    {
      if (tank->dir == RIGHT || tank->dir == LEFT)
      {
        swap(&tank->size_x, &tank->size_y);
      }

      if (joystick_direction.y < 0)
      {
        tank->dir = UP;
      }
      if (joystick_direction.y > 0)
      {
        tank->dir = DOWN;
      }

      tank->pos.y += tank->speed * joystick_direction.y;
    }
}

int master_loop()
{
  if (wifi_start_tcp(sizeof(tank_t), sizeof(tank_t), 1) != 0)
    return -__LINE__;

  tank_t master_tank = {
    .pos = {
      .x = TFT9341_WIDTH / 2,
      .y  = TFT9341_HEIGHT / 2
    },
    .size_x = 15,
    .size_y = 10,
    .dir = LEFT,
    .speed = 2,
  };

  tank_t slave_tank = {0};

  while (1)
  {
    draw_tank(&master_tank, TFT9341_BLACK);
    draw_tank(&slave_tank, TFT9341_BLACK);

    wifi_set_tx(&master_tank);
    wifi_get_rx(&slave_tank);
    
    get_new_position(&master_tank);

    draw_tank(&master_tank, TFT9341_WHITE);
    draw_tank(&slave_tank, TFT9341_WHITE);

    vTaskDelay(pdMS_TO_TICKS(50));
  }

  return 0;
}

int slave_loop()
{
  wifi_start_tcp(sizeof(tank_t), sizeof(tank_t), 0);
  
  
  tank_t master_tank = {
    .pos = {
      .x = TFT9341_WIDTH / 2,
      .y  = TFT9341_HEIGHT / 2
    },
    .size_x = 15,
    .size_y = 10,
    .dir = LEFT,
    .speed = 2,
  };

  tank_t slave_tank = {0};

  while (1)
  {
    draw_tank(&master_tank, TFT9341_BLACK);
    draw_tank(&slave_tank, TFT9341_BLACK);

    wifi_set_tx(&master_tank);
    wifi_get_rx(&slave_tank);

    get_new_position(&master_tank);

    draw_tank(&master_tank, TFT9341_WHITE);
    draw_tank(&slave_tank, TFT9341_WHITE);

    vTaskDelay(pdMS_TO_TICKS(50));
  }

  return 0;
}

int tanks_game(void* params)
{
  int is_master = (int)params;
  display_fill(TFT9341_BLACK);

  button_flush(BUTTON_RED);
  button_flush(BUTTON_BLUE);

  return is_master ? master_loop() : slave_loop();
}