#include "pong.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "display_api.h"
#include "joystick.h"
#include "wifi.h"
#include "button_pad.h"

//------------------------------------------------
inline void DrawBall(uint32_t x, uint32_t y, uint16_t color)
{
  DisplayFillRect(x, y, x + 5, y + 5, color);
}

static QueueHandle_t button_queue = NULL;

static void red_button_intr(void *pvParametrs)
{
  uint32_t button_pressed = 1;

  ESP_LOGI("PONG", "BUTTON PRESSED");
  xQueueSend(button_queue, &button_pressed, pdMS_TO_TICKS(20));
}

int PongGameLoop(void* pvParams)
{
  int is_owner = (int)pvParams;
  socket_t sock = 0;

  if (is_owner)
  {
    sock = wifi_accept();
  }
  else
  {
    sock = wifi_connect();
  }

  button_queue = xQueueCreate(128, sizeof(uint32_t));

  button_register_intr(BUTTON_RED, red_button_intr, NULL);

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

  int32_t ball_speed_x = 6;
  int32_t ball_speed_y = 0;

  int16_t ball_x = TFT9341_WIDTH / 2;
  int16_t ball_y = TFT9341_HEIGHT / 2;

  uint32_t wifi_tx_size;
  uint32_t button_pressed = 0;

  int ret = 0;

  while (1)
  {
    if (xQueueReceive(button_queue, &button_pressed, pdMS_TO_TICKS(20)) == pdTRUE)
    {
      if (button_pressed == 1)
      {
        return 0;
      }
    }

    joystick_data_get(&joystick_pos);

    if (is_owner)
    {
      // ret = wifi_get_rx(&ext_player, &wifi_tx_size);
      // if (ret != 0)
      // {
      //   memset(&ext_player, 0, sizeof(ext_player));
      // }
      // wifi_set_tx(&joystick_pos, sizeof(joystick_pos));

      if (wifi_transmit(sock, &joystick_pos, sizeof(joystick_pos)) != 0)
      {
        return -__LINE__;
      }
      int32_t recv_size = 0;
      if (wifi_recieve(sock, &ext_player, &recv_size, sizeof(joystick_pos)) != 0)
      {
        return -__LINE__;
      }

      if (recv_size != sizeof(ext_player))
      {
        return -__LINE__;
      }
    }
    else
    {
      int32_t recv_size = 0;
      if (wifi_recieve(sock, &ext_player, &recv_size, sizeof(joystick_pos)) != 0)
      {
        return -__LINE__;
      }

      if (recv_size != sizeof(ext_player))
      {
        return -__LINE__;
      }

      if (wifi_transmit(sock, &joystick_pos, sizeof(joystick_pos)) != 0)
      {
        return -__LINE__;
      }
      // wifi_set_tx(&joystick_pos, sizeof(joystick_pos));
      // ret = wifi_get_rx(&ext_player, &wifi_tx_size);
      // if (ret != 0)
      // {
      //   memset(&ext_player, 0, sizeof(ext_player));
      // }
    }
    

    
    // ESP_LOGI("PONG", "wifi ret %d", ret);

    // ESP_LOGI("PONG", "Ext player y: %d", ext_pos_y);
    // ESP_LOGI("PONG", "new frame");

    // if (TFT9341_WIDTH / 2 + 5 <= ball_x || ball_x <= TFT9341_WIDTH / 2 + 5)
    // {
    //   DisplayDrawLine(TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);
    // }

    // DrawBall(ball_x, ball_y, TFT9341_BLACK);

    // if (ball_x >= pos_x - 5 && pos_y <= ball_y && ball_y <= pos_y + 35)
    // {
    //   if (ball_y - pos_y <= 20)
    //   {
    //     ball_speed_y = -2;
    //   }
    //   else if (ball_y - pos_y >= 22)
    //   {
    //     ball_speed_y = 2;
    //   }
    //   else if (ball_y - pos_y == 21)
    //   {
    //     ball_speed_y = 0;
    //   }

    //   ball_speed_x = -ball_speed_x;
    // }
    // else if (ball_x <= ext_pos_x + 3 && ext_pos_y <= ball_y && ball_y <= ext_pos_y + 35)
    // {
    //   if (ball_y - ext_pos_y <= 20)
    //   {
    //     ball_speed_y = -2;
    //   }
    //   else if (ball_y - ext_pos_y >= 22)
    //   {
    //     ball_speed_y = 2;
    //   }
    //   else if (ball_y - ext_pos_y == 21)
    //   {
    //     ball_speed_y = 0;
    //   }

    //   ball_speed_x = -ball_speed_x;
    // }
    // else if (ball_x <= 0 || ball_x >= TFT9341_WIDTH - 5)
    // {
    //   ball_speed_x = -ball_speed_x;
    // }
    // else if (ball_y <= 30 || ball_y >= TFT9341_HEIGHT - 5)
    // {
    //   ball_speed_y = -ball_speed_y;
    // }

    // if (ball_x <= 0)
    // {
    //   score1++;
    //   if (score1 == '9' + 1)
    //   {
    //     score1 = '0';
    //   }

    //   DisplayDrawSymbol(TFT9341_WIDTH / 2 - 40, 5, score1);
    // }
    // else if (ball_x >= TFT9341_WIDTH - 5)
    // {
    //   score2++;
    //   if (score2 == '9' + 1)
    //   {
    //     score2 = '0';
    //   }

    //   DisplayDrawSymbol(TFT9341_WIDTH / 2 + 40, 5, score2);
    // }

    // ball_x += ball_speed_x;
    // ball_y += ball_speed_y;

    // DrawBall(ball_x, ball_y, TFT9341_WHITE);

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

    // ESP_LOGI("PONG", "work");
    ESP_LOGI("PONG", "joystick pos x: %d, y: %d", pos_x, pos_y);
    ESP_LOGI("PONG", "ext pos x: %d, y: %d", ext_pos_x, ext_pos_y);
    ESP_LOGI("PONG", "ball pos x: %d, y: %d", ball_x, ball_y);


    vTaskDelay(pdMS_TO_TICKS(50));
  }
  return 0;
}