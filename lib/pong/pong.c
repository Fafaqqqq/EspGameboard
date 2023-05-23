#include "pong.h"

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
inline void DrawBall(uint32_t x, uint32_t y, uint16_t color)
{
  DisplayFillRect(x, y, x + BALL_SIZE_X, y + BALL_SIZE_Y, color);
}

static QueueHandle_t button_queue = NULL;
static QueueHandle_t tcp_tx_queue = NULL;
static QueueHandle_t tcp_rx_queue = NULL;

static void red_button_intr(void *pvParametrs)
{
  uint32_t button_pressed = 1;

  xQueueSend(button_queue, &button_pressed, pdMS_TO_TICKS(20));
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
  uint8_t owner_score;
  uint8_t other_score;
} owner_data_t;

// void owner_wifi_task(void* pvParams)
// {
//   socket_t* sock = (socket_t*)pvParams;

//   owner_data_t data;
//   int32_t platform_y;
//   int32_t recv_size;

//   ESP_LOGI("owner_wifi_task", "sock %d", *sock);

//   while (1)
//   {
//     // xQueueReceive(tcp_tx_queue, &data, pdMS_TO_TICKS(5));

//     if (xQueueReceive(tcp_tx_queue, &data, pdMS_TO_TICKS(5)) == pdTRUE)
//     {
//       ESP_LOGI("owner_wifi_task", "recv queue data");
//       wifi_transmit(*sock, &data, sizeof(data));
//     }
//     else
//     {
//       ESP_LOGI("owner_wifi_task", "recv queue err");
//     }

    
//     // xQueueSend(tcp_rx_queue, &platform_y, pdMS_TO_TICKS(5));
//     if (xQueueSend(tcp_rx_queue, &platform_y, pdMS_TO_TICKS(5)) == pdTRUE)
//     {
//       ESP_LOGI("owner_wifi_task", "send queue data");
//       wifi_recieve(*sock, &(platform_y), &recv_size, sizeof(platform_y));
//     }
//     else
//     {
//       ESP_LOGI("owner_wifi_task", "send queue err");
//     }


//     vTaskDelay(pdMS_TO_TICKS(20));
//   }
// }

// void connect_wifi_task(void* pvParams)
// {
//   socket_t* sock = (socket_t*)pvParams;

//   owner_data_t data;
//   int32_t platform_y;
//   int32_t recv_size;

//   while (1)
//   {
//     if (xQueueReceive(tcp_tx_queue, &platform_y, pdMS_TO_TICKS(5)) == pdTRUE)
//     {
//       ESP_LOGI("connect_wifi_task", "recv queue data");
//     }
//     else
//     {
//       ESP_LOGI("connect_wifi_task", "recv queue err");
//     }

//     wifi_transmit(*sock, platform_y, sizeof(platform_y));
//     wifi_recieve(*sock, &(data), &recv_size, sizeof(data));
//     if (xQueueSend(tcp_rx_queue, &data, pdMS_TO_TICKS(5)) == pdTRUE)
//     {
//       ESP_LOGI("connect_wifi_task", "send queue data");
//     }
//     else
//     {
//       ESP_LOGI("connect_wifi_task", "send queue err");
//     }


//     vTaskDelay(pdMS_TO_TICKS(20));
//   }
// }

static int owner_loop(socket_t sock)
{
  int button_pressed = 0;

  uint8_t  self_score = '0';
  uint8_t other_score = '0';

  DisplaySetTextColor(TFT9341_WHITE);
  DisplaySetBackColor(TFT9341_BLACK);
  DisplaySetFont(&Font20);

  DisplayDrawSymbol(TFT9341_WIDTH / 2 - 40, 5, other_score);
  DisplayDrawSymbol(TFT9341_WIDTH / 2 + 40, 5, self_score);
  DisplayDrawLine(TFT9341_WHITE, 0, 29, TFT9341_WIDTH, 29);

  owner_data_t owner_data;
  memset(&owner_data, 0, sizeof(owner_data));

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

  DrawBall(ball_position.x, ball_position.y, TFT9341_WHITE);

  DisplayDrawLine(TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);

  DisplayFillRect
  (
    left_platform.x,
    left_platform.y,
    left_platform.x + PLATFORM_SIZE_X, 
    left_platform.y + PLATFORM_SIZE_Y, 
    TFT9341_WHITE
  );

  // xTaskCreatePinnedToCore(owner_wifi_task, "owner_wifi_task", 4096, &sock, 4, NULL, 1);

  while (1)
  {
    if (xQueueReceive(button_queue, &button_pressed, pdMS_TO_TICKS(1)) == pdTRUE)
    {
      if (button_pressed == 1)
      {
        return 0;
      }
    }

    memcpy(&(owner_data.other_score), &other_score,        sizeof(other_score));
    memcpy(&(owner_data.owner_score), &self_score,         sizeof(self_score));
    memcpy(&(owner_data.platform_y),  &(rigth_platform.y), sizeof(rigth_platform.y));
    memcpy(&(owner_data.ball),        &ball_position,      sizeof(ball_position));
    
    joystick_data_get(&joystick_direction);

    int32_t recv_size = 0;
    if (wifi_transmit(sock, &owner_data, sizeof(owner_data)) != 0) return -__LINE__;
    if (wifi_recieve(sock, &(left_platform.y), &recv_size, sizeof(left_platform.y)) != 0) return -__LINE__;
    ESP_LOGI("PONG", "recieve %ld bytes from %d", recv_size, sizeof(owner_data));
    if (recv_size != sizeof(left_platform.y)) return -__LINE__;
    // xQueueSend(tcp_tx_queue, &(left_platform.y), pdMS_TO_TICKS(5));
    // xQueueReceive(tcp_rx_queue, &(owner_data), pdMS_TO_TICKS(5));

    DrawBall( ball_position.x, ball_position.y, TFT9341_BLACK);

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
      self_score++;
      if (self_score == '9' + 1)
      {
        self_score = '0';
      }

      DisplayDrawSymbol(TFT9341_WIDTH / 2 + 40, 5, self_score);

      ball_position.x = 0;
      ball_speed.x = -ball_speed.x;
    }
    else if (ball_position.x + BALL_SIZE_X + ball_speed.x >= TFT9341_WIDTH - 1)
    {
      other_score++;
      if (other_score == '9' + 1)
      {
        other_score = '0';
      }

      DisplayDrawSymbol(TFT9341_WIDTH / 2 - 40, 5, other_score);

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
      // if (ball_position.x <= 3)
      // {
      //   self_score++;
      //   if (self_score == '9' + 1)
      //   {
      //     self_score = '0';
      //   }

      //   DisplayDrawSymbol(TFT9341_WIDTH / 2 + 40, 5, self_score);
      // }
      // else if (ball_position.x >= TFT9341_WIDTH - 6)
      // {
      //   other_score++;
      //   if (other_score == '9' + 1)
      //   {
      //     other_score = '0';
      //   }

      //   DisplayDrawSymbol(TFT9341_WIDTH / 2 - 40, 5, other_score);
      // }

      ball_position.x += ball_speed.x;
      ball_position.y += ball_speed.y;
    }

    

    DrawBall(ball_position.x, ball_position.y, TFT9341_WHITE);

    if (TFT9341_WIDTH / 2 - 5 <= ball_position.x  || ball_position.x  <= TFT9341_WIDTH / 2 + 5)
    {
      DisplayDrawLine(TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);
    }
    
    if (rigth_platform.y + joystick_direction.y * platform_speed >= 30 &&
        rigth_platform.y + joystick_direction.y * platform_speed <= TFT9341_HEIGHT - PLATFORM_SIZE_Y - 2)
    {
      DisplayFillRect
      (
        rigth_platform.x, 
        rigth_platform.y,
        rigth_platform.x + PLATFORM_SIZE_X,
        rigth_platform.y + PLATFORM_SIZE_Y,
        TFT9341_BLACK
      );

      rigth_platform.y += joystick_direction.y * platform_speed;

      DisplayFillRect
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
      DisplayFillRect
      (
        old_left_platform.x,
        old_left_platform.y,
        old_left_platform.x + PLATFORM_SIZE_X, 
        old_left_platform.y + PLATFORM_SIZE_Y, 
        TFT9341_BLACK
      );

      DisplayFillRect
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

static int connected_loop(socket_t sock)
{
  int button_pressed = 0;
  DisplaySetTextColor(TFT9341_WHITE);
  DisplaySetBackColor(TFT9341_BLACK);
  DisplaySetFont(&Font20);

  // DisplayDrawSymbol(TFT9341_WIDTH / 2 - 40, 5, other_score);
  // DisplayDrawSymbol(TFT9341_WIDTH / 2 + 40, 5, self_score);
  DisplayDrawLine(TFT9341_WHITE, 0, 29, TFT9341_WIDTH, 29);

  owner_data_t owner_data;
  memset(&owner_data, 0, sizeof(owner_data));

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

  // xTaskCreatePinnedToCore(connect_wifi_task, "connect_wifi_task", 4096, &sock, 4, NULL, 1);

  DisplayFillRect
  (
    left_platform.x,
    left_platform.y,
    left_platform.x + PLATFORM_SIZE_X, 
    left_platform.y + PLATFORM_SIZE_Y, 
    TFT9341_WHITE
  );

  DisplayDrawLine(TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);


  while (1)
  {
    joystick_data_get(&joystick_direction);

    if (xQueueReceive(button_queue, &button_pressed, pdMS_TO_TICKS(1)) == pdTRUE)
    {
      if (button_pressed == 1)
      {
        return 0;
      }
    }
    
    int32_t recv_size = 0;
    if (wifi_transmit(sock, &(rigth_platform.y), sizeof(rigth_platform.y)) != 0) return -__LINE__;
    if (wifi_recieve(sock, &owner_data, &recv_size, sizeof(owner_data)) != 0) return -__LINE__;
    ESP_LOGI("PONG", "recieve %ld bytes from %d", recv_size, sizeof(owner_data));
    if (recv_size != sizeof(owner_data)) return -__LINE__;

    // xQueueSend(tcp_tx_queue, &owner_data, pdMS_TO_TICKS(5));
    // xQueueReceive(tcp_rx_queue, &(left_platform.y), pdMS_TO_TICKS(5));

    memcpy(&(left_platform.y), &(owner_data.platform_y), sizeof(owner_data.platform_y));
    memcpy(&ball_position, &(owner_data.ball), sizeof(owner_data.ball));

    DisplayDrawSymbol(TFT9341_WIDTH / 2 + 40, 5, owner_data.owner_score);
    DisplayDrawSymbol(TFT9341_WIDTH / 2 - 40, 5, owner_data.other_score);
  
    if (TFT9341_WIDTH / 2 - 5 <= ball_position.x  || ball_position.x  <= TFT9341_WIDTH / 2 + 5)
    {
      DisplayDrawLine(TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);
    }

    if (old_ball_position.x != ball_position.x &&
        old_ball_position.y != ball_position.y)
    {
      DrawBall(TFT9341_WIDTH - old_ball_position.x, old_ball_position.y, TFT9341_BLACK);

      DrawBall(TFT9341_WIDTH - ball_position.x, ball_position.y, TFT9341_WHITE);

      memcpy(&old_ball_position, &ball_position, sizeof(ball_position));
    }

    if (rigth_platform.y + joystick_direction.y * platform_speed >= 30 &&
        rigth_platform.y + joystick_direction.y * platform_speed <= TFT9341_HEIGHT - PLATFORM_SIZE_Y - 2)
    {
      DisplayFillRect
      (
        rigth_platform.x, 
        rigth_platform.y,
        rigth_platform.x + PLATFORM_SIZE_X,
        rigth_platform.y + PLATFORM_SIZE_Y,
        TFT9341_BLACK
      );

      rigth_platform.y += joystick_direction.y * platform_speed;

      DisplayFillRect
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
      DisplayFillRect
      (
        old_left_platform.x,
        old_left_platform.y,
        old_left_platform.x + PLATFORM_SIZE_X, 
        old_left_platform.y + PLATFORM_SIZE_Y, 
        TFT9341_BLACK
      );

      DisplayFillRect
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
  button_queue = xQueueCreate(128, sizeof(uint32_t));

  int      owner = (int)pvParams;
  socket_t sock = owner ? wifi_accept() : wifi_connect();

  ESP_LOGI("pong_game", "sock %d", sock);


  button_register_intr(BUTTON_RED, red_button_intr, NULL);
  // if (owner)
  // {
  //   tcp_tx_queue = xQueueCreate(128, sizeof(owner_data_t));
  //   tcp_rx_queue = xQueueCreate(128, sizeof(int32_t));
  // }
  // else
  // {
  //   tcp_tx_queue = xQueueCreate(128, sizeof(int32_t));
  //   tcp_rx_queue = xQueueCreate(128, sizeof(owner_data_t));
  // }

  DisplayFill(TFT9341_BLACK);

  return owner ? owner_loop(sock) : connected_loop(sock);
}