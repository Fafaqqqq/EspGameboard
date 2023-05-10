#include "main.h"

//------------------------------------------------
static const char *TAG = "main";

//------------------------------------------------
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
  int dc = (int)t->user;
  gpio_set_level(CONFIG_PIN_NUM_DC, dc);
}

//------------------------------------------------
void DrawBall(spi_device_handle_t spi, uint32_t x, uint32_t y, uint16_t color)
{
  TFT9341_FillRect(spi, x, y, x + 5, y + 5, color);
}

//------------------------------------------------
void app_main(void)
{
  joystick_controller_init();
  wifi_init();

  esp_err_t ret;
  spi_device_handle_t spi;

  // Configure SPI bus
  spi_bus_config_t cfg =
  {
      .mosi_io_num = CONFIG_PIN_NUM_MOSI,
      .miso_io_num = -1,
      .sclk_io_num = CONFIG_PIN_NUM_CLK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 16 * 320 * 2 + 8,
  };
  ret = spi_bus_initialize(HSPI_HOST, &cfg, SPI_DMA_CH_AUTO);
  ESP_LOGI(TAG, "spi bus initialize: %d", ret);

  spi_device_interface_config_t devcfg =
  {
      .clock_speed_hz = 10 * 1000 * 1000,      // Clock out at 10 MHz
      .mode = 0,                               // SPI mode 0
      .spics_io_num = CONFIG_PIN_NUM_CS,       // CS pin
      .queue_size = 7,                         // We want to be able to queue 7 transactions at a time
      .pre_cb = lcd_spi_pre_transfer_callback, // Specify pre-transfer callback to handle D/C line
  };
  ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
  ESP_LOGI(TAG, "spi bus add device: %d", ret);

  TFT9341_ini(spi, 320, 240);
  TFT9341_FillScreen(spi, TFT9341_BLACK);

  joystick_data_t joystick_pos;

  uint16_t pos_x = TFT9341_WIDTH - 10;
  uint16_t pos_y = TFT9341_HEIGHT / 2;

  float speed = 4.0f;

  TFT9341_SetTextColor(TFT9341_WHITE);
  TFT9341_SetBackColor(TFT9341_BLACK);
  TFT9341_SetFont(&Font20);

  uint8_t score1 = '0';
  uint8_t score2 = '0';

  // int cnt = 200;

  // TFT9341_DrawLine(spi, TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);
  TFT9341_DrawChar(spi, TFT9341_WIDTH / 2 - 40, 5, score1);
  TFT9341_DrawChar(spi, TFT9341_WIDTH / 2 + 40, 5, score2);
  TFT9341_DrawLine(spi, TFT9341_WHITE, 0, 29, TFT9341_WIDTH, 29);

  int32_t ball_speed_x = 2;
  int32_t ball_speed_y = 0;

  int16_t ball_x = TFT9341_WIDTH / 2;
  int16_t ball_y = TFT9341_HEIGHT / 2;

  while (1)
  {
    joystick_data_get(&joystick_pos);

    if (TFT9341_WIDTH / 2 + 5 <= ball_x || ball_x <= TFT9341_WIDTH / 2 + 5)
    {
      TFT9341_DrawLine(spi, TFT9341_WHITE, TFT9341_WIDTH / 2, 0, TFT9341_WIDTH / 2, TFT9341_HEIGHT);
    }

    DrawBall(spi, ball_x, ball_y, TFT9341_BLACK);

    if ((ball_x >= pos_x - 5 || ball_x <= 20 + 3) && (pos_y <= ball_y && ball_y <= pos_y + 35))
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
    else if ((ball_x <= 0 || ball_x >= TFT9341_WIDTH - 5))
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

      TFT9341_DrawChar(spi, TFT9341_WIDTH / 2 - 40, 5, score1);
    }
    else if (ball_x >= TFT9341_WIDTH - 5)
    {
      score2++;
      if (score2 == '9' + 1)
      {
        score2 = '0';
      }

      TFT9341_DrawChar(spi, TFT9341_WIDTH / 2 + 40, 5, score2);
    }

    ball_x += ball_speed_x;
    ball_y += ball_speed_y;

    DrawBall(spi, ball_x, ball_y, TFT9341_WHITE);

    if (pos_y + joystick_pos.y * speed <= 30 || pos_y + joystick_pos.y * speed > TFT9341_HEIGHT - 43)
    {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    TFT9341_FillRect(spi, pos_x, pos_y, pos_x + 3, pos_y + 41, TFT9341_BLACK);
    TFT9341_FillRect(spi, 20, pos_y, 20 + 3, pos_y + 41, TFT9341_BLACK);

    pos_y += joystick_pos.y * speed;

    TFT9341_FillRect(spi, pos_x, pos_y, pos_x + 3, pos_y + 41, TFT9341_WHITE);
    TFT9341_FillRect(spi, 20, pos_y, 20 + 3, pos_y + 41, TFT9341_WHITE);

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
//------------------------------------------------
