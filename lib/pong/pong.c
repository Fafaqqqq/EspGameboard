#include "pong.h"

#include "display_api.h"
#include "joystick.h"
#include "wifi.h"

//------------------------------------------------
inline void DrawBall(uint32_t x, uint32_t y, uint16_t color)
{
  DisplayFillRect(x, y, x + 5, y + 5, color);
}

int PongGameLoop()
{
  DisplayFill(TFT9341_BLACK);

  joystick_data_t joystick_pos;
  uint16_t pos_x = TFT9341_WIDTH - 8;
  uint16_t pos_y = TFT9341_HEIGHT / 2;

  joystick_data_t ext_player;
  uint16_t ext_pos_x = 5;
  uint16_t ext_pos_y = TFT9341_HEIGHT / 2;

  float speed = 4.0f;

  DisplaySetTextColor(TFT9341_WHITE);
  DisplaySetBackColor(TFT9341_BLACK);
  DisplaySetFont(&Font20);

  uint8_t score1 = '0';
  uint8_t score2 = '0';

  DisplayDrawSymbol(TFT9341_WIDTH / 2 - 40, 5, score1);
  DisplayDrawSymbol(TFT9341_WIDTH / 2 + 40, 5, score2);
  DisplayDrawLine(TFT9341_WHITE, 0, 29, TFT9341_WIDTH, 29);

  int32_t ball_speed_x = 2;
  int32_t ball_speed_y = 0;

  int16_t ball_x = TFT9341_WIDTH / 2;
  int16_t ball_y = TFT9341_HEIGHT / 2;

  uint32_t        wifi_tx_size;

  while (1)
  {
    joystick_data_get(&joystick_pos);

    int ret = wifi_get_rx(&ext_player, &wifi_tx_size);
    if (ret != 0)
    {
      memset(&ext_player, 0, sizeof(ext_player));
    }
    ret = wifi_set_tx(&joystick_pos, sizeof(joystick_pos));
    

    if (TFT9341_WIDTH / 2 + 5 <= ball_x || ball_x <= TFT9341_WIDTH / 2 + 5)
    {
      DisplayDrawLine(TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);
    }

    DrawBall(ball_x, ball_y, TFT9341_BLACK);

    if (ball_x >= pos_x - 5 && pos_y <= ball_y && ball_y <= pos_y + 35)
    {
      if (ball_y - pos_y <= 20)
      {
        ball_speed_y = -2;
      }
      else if (ball_y - pos_y >= 22)
      {
        ball_speed_y = 2;
      }
      else if (ball_y - pos_y == 21)
      {
        ball_speed_y = 0;
      }

      ball_speed_x = -ball_speed_x;
    }
    else if (ball_x <= ext_pos_x + 3 && ext_pos_y <= ball_y && ball_y <= ext_pos_y + 35)
    {
      if (ball_y - ext_pos_y <= 20)
      {
        ball_speed_y = -2;
      }
      else if (ball_y - ext_pos_y >= 22)
      {
        ball_speed_y = 2;
      }
      else if (ball_y - ext_pos_y == 21)
      {
        ball_speed_y = 0;
      }

      ball_speed_x = -ball_speed_x;
    }
    else if (ball_x <= 0 || ball_x >= TFT9341_WIDTH - 5)
    {
      ball_speed_x = -ball_speed_x;
    }
    else if (ball_y <= 30 || ball_y >= TFT9341_HEIGHT - 5)
    {
      ball_speed_y = -ball_speed_y;
    }

    if (ball_x <= 0)
    {
      score1++;
      if (score1 == '9' + 1)
      {
        score1 = '0';
      }

      DisplayDrawSymbol(TFT9341_WIDTH / 2 - 40, 5, score1);
    }
    else if (ball_x >= TFT9341_WIDTH - 5)
    {
      score2++;
      if (score2 == '9' + 1)
      {
        score2 = '0';
      }

      DisplayDrawSymbol(TFT9341_WIDTH / 2 + 40, 5, score2);
    }

    ball_x += ball_speed_x;
    ball_y += ball_speed_y;

    DrawBall(ball_x, ball_y, TFT9341_WHITE);

    if (pos_y + joystick_pos.y * speed > 30 && pos_y + joystick_pos.y * speed <= TFT9341_HEIGHT - 43)
    {
      DisplayFillRect(pos_x, pos_y, pos_x + 3, pos_y + 41, TFT9341_BLACK);
      pos_y += joystick_pos.y * speed;
      DisplayFillRect(pos_x, pos_y, pos_x + 3, pos_y + 41, TFT9341_WHITE);
    }

    if (ext_pos_y + ext_player.y * speed > 30 && ext_pos_y + ext_player.y * speed <= TFT9341_HEIGHT - 43)
    {
      DisplayFillRect(ext_pos_x, ext_pos_y, ext_pos_x + 3, ext_pos_y + 41, TFT9341_BLACK);
      ext_pos_y += ext_player.y * speed;
      DisplayFillRect(ext_pos_x, ext_pos_y, ext_pos_x + 3, ext_pos_y + 41, TFT9341_WHITE);
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
  return 0;
}