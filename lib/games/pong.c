#include "games.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "display_api.h"
#include "joystick.h"
#include "wifi.h"
#include "button_pad.h"

#define PLATFORM_SIZE_X 4
#define PLATFORM_SIZE_Y 41

#define BALL_SIZE_X 5
#define BALL_SIZE_Y 5

//------------------------------------------------
inline void draw_ball(uint32_t x, uint32_t y, uint16_t color)
{
  display_fill_rect(x, y, x + BALL_SIZE_X, y + BALL_SIZE_Y, color);
}

typedef struct 
{
  int32_t x;
  int32_t y;
} point_t;


typedef struct 
{
  int32_t platform_y;
  point_t ball;
  uint8_t score;
  uint8_t slave_score;
} master_data_t;

static int master_loop(socket_t sock)
{
  uint8_t master_score = '0';
  uint8_t  slave_score = '0';

  display_set_text_color(TFT9341_WHITE);
  display_set_back_color(TFT9341_BLACK);
  display_set_font(&Font20);

  display_draw_symbol(TFT9341_WIDTH / 2 - 40, 5, slave_score);
  display_draw_symbol(TFT9341_WIDTH / 2 + 40, 5, master_score);
  display_draw_line(TFT9341_WHITE, 0, 29, TFT9341_WIDTH, 29);

  master_data_t master_data;
  memset(&master_data, 0, sizeof(master_data));

  joystick_data_t joystick_direction;
  memset(&joystick_direction, 0, sizeof(joystick_direction));

  point_t rigth_platform =
  {
    .x = TFT9341_WIDTH - PLATFORM_SIZE_X - 1,
    .y = TFT9341_HEIGHT / 2
  };

  point_t left_platform =
  {
    .x = 0,
    .y = TFT9341_HEIGHT / 2,
  };

  point_t old_left_platform =
  {
    .x = 0,
    .y = TFT9341_HEIGHT / 2,
  };


  point_t ball_position =
  {
    .x = TFT9341_WIDTH / 2,
    .y = TFT9341_HEIGHT / 2  + 15,
  };

  uint32_t  platform_speed = 6;
  point_t   ball_speed =
  {
    .x = 8,
    .y = 0,
  };

  draw_ball(ball_position.x, ball_position.y, TFT9341_WHITE);

  display_draw_line(TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);

  display_fill_rect
  (
    left_platform.x,
    left_platform.y,
    left_platform.x + PLATFORM_SIZE_X, 
    left_platform.y + PLATFORM_SIZE_Y, 
    TFT9341_WHITE
  );
  while (1)
  {
    if (button_pressed(BUTTON_RED))
      return 0;

    memcpy(&(master_data.slave_score), &slave_score,        sizeof(slave_score));
    memcpy(&(master_data.score), &master_score,         sizeof(master_score));
    memcpy(&(master_data.platform_y),  &(rigth_platform.y), sizeof(rigth_platform.y));
    memcpy(&(master_data.ball),        &ball_position,      sizeof(ball_position));
    
    joystick_data_get(&joystick_direction);

    int32_t recv_size = 0;
    if (wifi_transmit(sock, &master_data, sizeof(master_data)) != 0) 
      return -__LINE__;
    if (wifi_recieve(sock, &(left_platform.y), &recv_size, sizeof(left_platform.y)) != 0) 
      return -__LINE__;
    if (recv_size != sizeof(left_platform.y)) 
      return -__LINE__;

    draw_ball( ball_position.x, ball_position.y, TFT9341_BLACK);

    if (ball_position.x + ball_speed.x <= left_platform.x + PLATFORM_SIZE_X && 
        ball_position.y - BALL_SIZE_Y >= left_platform.y &&
        ball_position.y <= left_platform.y + PLATFORM_SIZE_Y)
    {
      ball_position.x = left_platform.x + PLATFORM_SIZE_X;
      ball_speed.x = -ball_speed.x;
      if (ball_speed.y == 0)
      {
        if (ball_position.y - left_platform.y <= 20)
        {
          ball_speed.y = -4;
        }
        else if (ball_position.y - left_platform.y >= 22)
        {
          ball_speed.y = 4;
        }
        else if (ball_position.y - left_platform.y == 21)
        {
          ball_speed.y = 0;
        }
      }
    }
    else if (ball_position.x + BALL_SIZE_X + ball_speed.x >= rigth_platform.x && 
             ball_position.y - BALL_SIZE_Y >= rigth_platform.y &&
             ball_position.y <= rigth_platform.y + PLATFORM_SIZE_Y)
    {
      ball_position.x = rigth_platform.x - BALL_SIZE_X - 1;
      ball_speed.x = -ball_speed.x;

      if (ball_speed.y == 0)
      {
        if (ball_position.y - rigth_platform.y <= 20)
        {
          ball_speed.y = -4;
        }
        else if (ball_position.y - rigth_platform.y >= 22)
        {
          ball_speed.y = 4;
        }
        else if (ball_position.y - rigth_platform.y == 21)
        {
          ball_speed.y = 0;
        }
      }
    }
    else if (ball_position.x + ball_speed.x <= 0)
    {
      master_score++;
      if (master_score == '9' + 1)
      {
        master_score = '0';
      }

      display_draw_symbol(TFT9341_WIDTH / 2 + 40, 5, master_score);

      ball_position.x = 0;
      ball_speed.x = -ball_speed.x;
    }
    else if (ball_position.x + BALL_SIZE_X + ball_speed.x >= TFT9341_WIDTH - 1)
    {
      slave_score++;
      if (slave_score == '9' + 1)
      {
        slave_score = '0';
      }

      display_draw_symbol(TFT9341_WIDTH / 2 - 40, 5, slave_score);

      ball_position.x = TFT9341_WIDTH - BALL_SIZE_X - 1;
      ball_speed.x = -ball_speed.x;
    }
    else if (ball_position.y + ball_speed.y <= 30)
    {
      ball_position.y = 30;
      ball_speed.y = -ball_speed.y;
    }
    else if (ball_position.y + BALL_SIZE_Y + ball_speed.y >= TFT9341_HEIGHT - 1)
    {
      ball_position.y = TFT9341_HEIGHT - BALL_SIZE_Y - 1;
      ball_speed.y = -ball_speed.y;
    }
    else
    {

      ball_position.x += ball_speed.x;
      ball_position.y += ball_speed.y;
    }

    draw_ball(ball_position.x, ball_position.y, TFT9341_WHITE);

    if (TFT9341_WIDTH / 2 - 5 <= ball_position.x  || ball_position.x  <= TFT9341_WIDTH / 2 + 5)
    {
      display_draw_line(TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);
    }
    
    if (rigth_platform.y + joystick_direction.y * platform_speed >= 30 &&
        rigth_platform.y + joystick_direction.y * platform_speed <= TFT9341_HEIGHT - PLATFORM_SIZE_Y - 2)
    {
      display_fill_rect
      (
        rigth_platform.x, 
        rigth_platform.y,
        rigth_platform.x + PLATFORM_SIZE_X,
        rigth_platform.y + PLATFORM_SIZE_Y,
        TFT9341_BLACK
      );

      rigth_platform.y += joystick_direction.y * platform_speed;

      display_fill_rect
      (
        rigth_platform.x, 
        rigth_platform.y, 
        rigth_platform.x + PLATFORM_SIZE_X, 
        rigth_platform.y + PLATFORM_SIZE_Y, 
        TFT9341_WHITE
      );
    }

    if (old_left_platform.y != left_platform.y)
    {
      display_fill_rect
      (
        old_left_platform.x,
        old_left_platform.y,
        old_left_platform.x + PLATFORM_SIZE_X, 
        old_left_platform.y + PLATFORM_SIZE_Y, 
        TFT9341_BLACK
      );

      display_fill_rect
      (
        left_platform.x,
        left_platform.y,
        left_platform.x + PLATFORM_SIZE_X, 
        left_platform.y + PLATFORM_SIZE_Y, 
        TFT9341_WHITE
      );

      memcpy(&old_left_platform, &left_platform, sizeof(left_platform));
    }

    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

static int slave_loop(socket_t sock)
{
  display_set_text_color(TFT9341_WHITE);
  display_set_back_color(TFT9341_BLACK);
  display_set_font(&Font20);

  display_draw_line(TFT9341_WHITE, 0, 29, TFT9341_WIDTH, 29);

  master_data_t master_data;
  memset(&master_data, 0, sizeof(master_data));

  joystick_data_t joystick_direction;
  memset(&joystick_direction, 0, sizeof(joystick_direction));

  point_t rigth_platform =
  {
    .x = TFT9341_WIDTH - PLATFORM_SIZE_X - 1,
    .y = TFT9341_HEIGHT / 2
  };

  point_t left_platform =
  {
    .x = 0,
    .y = TFT9341_HEIGHT / 2,
  };
  point_t old_left_platform =
  {
    .x = 0,
    .y = TFT9341_HEIGHT / 2,
  };

  point_t ball_position =
  {
    .x = TFT9341_WIDTH / 2,
    .y = TFT9341_HEIGHT / 2  + 15,
  };

  point_t old_ball_position =
  {
    .x = TFT9341_WIDTH / 2,
    .y = TFT9341_HEIGHT / 2  + 15,
  };

  uint32_t  platform_speed = 6;


  display_fill_rect
  (
    left_platform.x,
    left_platform.y,
    left_platform.x + PLATFORM_SIZE_X, 
    left_platform.y + PLATFORM_SIZE_Y, 
    TFT9341_WHITE
  );

  display_draw_line(TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);


  while (1)
  {
    if (button_pressed(BUTTON_RED))
      return 0;

    joystick_data_get(&joystick_direction);
  
    int32_t recv_size = 0;
    if (wifi_transmit(sock, &(rigth_platform.y), sizeof(rigth_platform.y)) != 0) 
      return -__LINE__;
    if (wifi_recieve(sock, &master_data, &recv_size, sizeof(master_data)) != 0) 
      return -__LINE__;
    if (recv_size != sizeof(master_data))
      return -__LINE__;

    memcpy(&(left_platform.y), &(master_data.platform_y), sizeof(master_data.platform_y));
    memcpy(&ball_position, &(master_data.ball), sizeof(master_data.ball));

    display_draw_symbol(TFT9341_WIDTH / 2 + 40, 5, master_data.score);
    display_draw_symbol(TFT9341_WIDTH / 2 - 40, 5, master_data.slave_score);
  
    if (TFT9341_WIDTH / 2 - 5 <= ball_position.x  || ball_position.x  <= TFT9341_WIDTH / 2 + 5)
    {
      display_draw_line(TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);
    }

    if (old_ball_position.x != ball_position.x &&
        old_ball_position.y != ball_position.y)
    {
      draw_ball(TFT9341_WIDTH - old_ball_position.x, old_ball_position.y, TFT9341_BLACK);

      draw_ball(TFT9341_WIDTH - ball_position.x, ball_position.y, TFT9341_WHITE);

      memcpy(&old_ball_position, &ball_position, sizeof(ball_position));
    }

    if (rigth_platform.y + joystick_direction.y * platform_speed >= 30 &&
        rigth_platform.y + joystick_direction.y * platform_speed <= TFT9341_HEIGHT - PLATFORM_SIZE_Y - 2)
    {
      display_fill_rect
      (
        rigth_platform.x, 
        rigth_platform.y,
        rigth_platform.x + PLATFORM_SIZE_X,
        rigth_platform.y + PLATFORM_SIZE_Y,
        TFT9341_BLACK
      );

      rigth_platform.y += joystick_direction.y * platform_speed;

      display_fill_rect
      (
        rigth_platform.x, 
        rigth_platform.y, 
        rigth_platform.x + PLATFORM_SIZE_X, 
        rigth_platform.y + PLATFORM_SIZE_Y, 
        TFT9341_WHITE
      );

    }

    if (old_left_platform.y != left_platform.y)
    {
      display_fill_rect
      (
        old_left_platform.x,
        old_left_platform.y,
        old_left_platform.x + PLATFORM_SIZE_X, 
        old_left_platform.y + PLATFORM_SIZE_Y, 
        TFT9341_BLACK
      );

      display_fill_rect
      (
        left_platform.x,
        left_platform.y,
        left_platform.x + PLATFORM_SIZE_X, 
        left_platform.y + PLATFORM_SIZE_Y, 
        TFT9341_WHITE
      );

      memcpy(&old_left_platform, &left_platform, sizeof(left_platform));
    }
  }
}

int pong_game(void* pvParams)
{
  button_flush(BUTTON_RED);
  button_flush(BUTTON_BLUE);

  
  int is_master = (int)pvParams;

  socket_t sock = is_master 
                ? wifi_accept()
                : wifi_connect();

  display_fill(TFT9341_BLACK);

  return is_master ? master_loop(sock) : slave_loop(sock);
}