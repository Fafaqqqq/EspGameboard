#include "spi_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <string.h>

#include "esp_log.h"

#define SPI_MAX_INIT_DATA_LENGTH 15
#define SPI_INIT_DATA_CNT 22

typedef struct
{
  uint8_t data[SPI_MAX_INIT_DATA_LENGTH];
  uint8_t lenght;
  spi_dev_controller_cmd_t cmd;
} spi_data_t;

static volatile SemaphoreHandle_t priv_mtx;

static spi_data_t _spi_init_data_buf[SPI_INIT_DATA_CNT] = {
    {.data = {0x39, 0x2C, 0x00, 0x34, 0x02},
     .lenght = 5,
     .cmd = 0xCB},
    {.data = {0x00, 0x83, 0x30},
     .lenght = 3,
     .cmd = 0xCF},
    {.data = {0x85, 0x01, 0x79},
     .lenght = 3,
     .cmd = 0xE8},
    {.data = {0x0, 0x0},
     .lenght = 2,
     .cmd = 0xEA},
    {.data = {0x64, 0x03, 0x12, 0x81},
     .lenght = 4,
     .cmd = 0xED},
    {.data = {0x20},
     .lenght = 1,
     .cmd = 0xF7},
    {.data = {0x26},
     .lenght = 1,
     .cmd = 0xC0},
    {.data = {0x11},
     .lenght = 1,
     .cmd = 0xC1},
    {.data = {0x35, 0x3E},
     .lenght = 2,
     .cmd = 0xC5},
    {.data = {0xBE},
     .lenght = 1,
     .cmd = 0xC7},
    {.data = {0x28},
     .lenght = 1,
     .cmd = 0x36},
    {.data = {0x55},
     .lenght = 1,
     .cmd = 0x3A},
    {.data = {0x00, 0x1B},
     .lenght = 2,
     .cmd = 0xB1},
    {
        .data = {0x08, 0x82, 0x27},
        .lenght = 3,
        .cmd = 0xB6,
    },
    {.data = {0x08},
     .lenght = 1,
     .cmd = 0xF2},
    {.data = {0x01},
     .lenght = 1,
     .cmd = 0x25},
    {.data = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00},
     .lenght = 15,
     .cmd = 0xE0},
    {.data = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F},
     .lenght = 15,
     .cmd = 0xE1},
    {.data = {0x00, 0x00, 0x00, 0xEF},
     .lenght = 4,
     .cmd = 0x2A},
    {.data = {0x00, 0x00, 0x01, 0x3F},
     .lenght = 4,
     .cmd = 0x2B},
    {.data = {0x07},
     .lenght = 1,
     .cmd = 0xB7}};

void spi_display_cmd_send(spi_device_handle_t spi, const uint8_t cmd)
{
  esp_err_t ret;
  spi_transaction_t spi_trans;

  memset(&spi_trans, 0, sizeof(spi_trans)); // Zero out the transaction

  spi_trans.length = 8;       // Command is 8 bits
  spi_trans.tx_buffer = &cmd; // The data is the cmd itself
  spi_trans.user = (void *)0; // D/C needs to be set to 0

  if (xSemaphoreTake(priv_mtx, pdMS_TO_TICKS(100)) == pdTRUE)
  {
    ret = spi_device_polling_transmit(spi, &spi_trans); // Transmit!
    xSemaphoreGive(priv_mtx);
    assert(ret == ESP_OK);                              // Should have had no issues.
  }
  else
  {
      ESP_LOGI("SPI", "Unable take mutex, print skip");
  }
}

void spi_display_data_send(spi_device_handle_t spi, const uint8_t *data, int len)
{
  esp_err_t ret;
  spi_transaction_t t;

  if (len == 0)
    return; // no need to send anything

  memset(&t, 0, sizeof(t)); // Zero out the transaction

  t.length = len * 8;                         // Len is in bytes, transaction length is in bits.
  t.tx_buffer = data;                         // Data
  t.user = (void *)1;                         // D/C needs to be set to 1

  if (xSemaphoreTake(priv_mtx, pdMS_TO_TICKS(100)) == pdTRUE)
  {
    ret = spi_device_polling_transmit(spi, &t); // Transmit!
    xSemaphoreGive(priv_mtx);
    assert(ret == ESP_OK);                              // Should have had no issues.
  }
  else
  {
      ESP_LOGI("SPI", "Unable take mutex, print skip");
  }
}

void spi_display_reset(void)
{
  gpio_set_level(CONFIG_PIN_NUM_RST, 0);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  gpio_set_level(CONFIG_PIN_NUM_RST, 1);
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

static void spi_display_send_hor_block(spi_device_handle_t spi, int ypos, uint16_t *data)
{
  static spi_transaction_t trans[6];
  esp_err_t ret;

  int x;
  for (x = 0; x < 6; x++)
  {
    memset(&trans[x], 0, sizeof(spi_transaction_t));
    if ((x & 1) == 0)
    {
      // Even transfers are commands
      trans[x].length = 8;
      trans[x].user = (void *)0;
    }
    else
    {
      // Odd transfers are data
      trans[x].length = 8 * 4;
      trans[x].user = (void *)1;
    }
    trans[x].flags = SPI_TRANS_USE_TXDATA;
  }

  trans[0].tx_data[0] = 0x2A;                    // Column Address Set
  trans[1].tx_data[0] = 0;                       // Start Col High
  trans[1].tx_data[1] = 0;                       // Start Col Low
  trans[1].tx_data[2] = (DISPLAY_SIZE_X) >> 8;   // End Col High
  trans[1].tx_data[3] = (DISPLAY_SIZE_X)&0xff;   // End Col Low
  trans[2].tx_data[0] = 0x2B;                    // Page address set
  trans[3].tx_data[0] = ypos >> 8;               // Start page high
  trans[3].tx_data[1] = ypos & 0xff;             // start page low
  trans[3].tx_data[2] = (ypos + 16) >> 8;        // end page high
  trans[3].tx_data[3] = (ypos + 16) & 0xff;      // end page low
  trans[4].tx_data[0] = 0x2C;                    // memory write
  trans[5].tx_buffer = data;                     // finally send the line data
  trans[5].length = DISPLAY_SIZE_X * 2 * 8 * 16; // Data length, in bits
  trans[5].flags = 0;                            // undo SPI_TRANS_USE_TXDATA flag
  for (x = 0; x < 6; x++)
  {
    ret = spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}

static void send_blocks(spi_device_handle_t spi, int x1, int y1,
                        int x2, int y2, uint16_t *data)
{
  esp_err_t ret;
  int x;
  static spi_transaction_t trans[6];
  for (x = 0; x < 6; x++)
  {
    memset(&trans[x], 0, sizeof(spi_transaction_t));
    if ((x & 1) == 0)
    {
      // Even transfers are commands
      trans[x].length = 8;
      trans[x].user = (void *)0;
    }
    else
    {
      // Odd transfers are data
      trans[x].length = 8 * 4;
      trans[x].user = (void *)1;
    }
    trans[x].flags = SPI_TRANS_USE_TXDATA;
  }
  trans[0].tx_data[0] = 0x2A;                              // Column Address Set
  trans[1].tx_data[0] = (x1 >> 8) & 0xFF;                  // Start Col High
  trans[1].tx_data[1] = x1 & 0xFF;                         // Start Col Low
  trans[1].tx_data[2] = (x2 >> 8) & 0xFF;                  // End Col High
  trans[1].tx_data[3] = x2 & 0xFF;                         // End Col Low
  trans[2].tx_data[0] = 0x2B;                              // Page address set
  trans[3].tx_data[0] = (y1 >> 8) & 0xFF;                  // Start page high
  trans[3].tx_data[1] = y1 & 0xFF;                         // start page low
  trans[3].tx_data[2] = (y2 >> 8) & 0xFF;                  // end page high
  trans[3].tx_data[3] = y2 & 0xFF;                         // end page low
  trans[4].tx_data[0] = 0x2C;                              // memory write
  trans[5].tx_buffer = data;                               // finally send the line data
  trans[5].length = (x2 - x1 + 1) * (y2 - y1 + 1) * 2 * 8; // Data length, in bits
  trans[5].flags = 0;                                      // undo SPI_TRANS_USE_TXDATA flag
  for (x = 0; x < 6; x++)
  {
    ret = spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}
//-------------------------------------------------------------------
static void spi_display_block_finish(spi_device_handle_t spi)
{
  spi_transaction_t *rtrans;
  esp_err_t ret;
  // Wait for all 6 transactions to be done and get back the results.
  for (int x = 0; x < 6; x++)
  {
    ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
    assert(ret == ESP_OK);
    // We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
  }
}

void spi_display_fill(spi_device_handle_t spi, uint16_t color)
{
  // Сделать статическое выделение памяти!
  uint16_t *blck = heap_caps_malloc(DISPLAY_SIZE_X * 16 * sizeof(uint16_t), MALLOC_CAP_DMA);

  color = color << 8 | color >> 8;

  for (int i = 0; i < 16; i++)
  {
    for (int j = 0; j < DISPLAY_SIZE_X; j++)
    {
      blck[i * DISPLAY_SIZE_X + j] = color;
    }
  }

  for (int i = 0; i < DISPLAY_SIZE_X / 16; i++)
  {
    spi_display_send_hor_block(spi, i * 16, blck);
    spi_display_block_finish(spi);
  }

  heap_caps_free(blck);
}

static void spi_display_write_data(spi_device_handle_t spi, uint8_t *buff, size_t buff_size)
{
  esp_err_t ret;
  spi_transaction_t t;
  while (buff_size > 0)
  {
    uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;

    if (chunk_size == 0)
      return;                                   // no need to send anything

    memset(&t, 0, sizeof(t));                   // Zero out the transaction
    
    t.length = chunk_size * 8;                  // Len is in bytes, transaction length is in bits.
    t.tx_buffer = buff;                         // Data
    t.user = (void *)1;                         // D/C needs to be set to 1

    ret = spi_device_polling_transmit(spi, &t); // Transmit!
    assert(ret == ESP_OK);                      // Should have had no issues.

    buff += chunk_size;
    buff_size -= chunk_size;
  }
}

static void spi_display_set_add_wind(spi_device_handle_t spi, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  // column address set
  spi_display_cmd_send(spi, 0x2A); // CASET
  {
    uint8_t data[] = {(x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF};
    spi_display_write_data(spi, data, sizeof(data));
  }
  // row address set
  spi_display_cmd_send(spi, 0x2B); // RASET
  {
    uint8_t data[] = {(y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF};
    spi_display_write_data(spi, data, sizeof(data));
  }
  // write to RAM
  spi_display_cmd_send(spi, 0x2C); // RAMWR
}

void spi_display_draw_pixel(spi_device_handle_t spi, int x, int y, uint16_t color)
{
  uint8_t data[2];
  if ((x < 0) || (y < 0) || (x >= DISPLAY_SIZE_Y) || (y >= DISPLAY_SIZE_X))
    return;
  data[0] = color >> 8;
  data[1] = color & 0xFF;
  // if (xSemaphoreTake(priv_mtx, pdMS_TO_TICKS(100)) == pdTRUE)
  // {
    spi_display_set_add_wind(spi, x, y, x, y);
    spi_display_cmd_send(spi, 0x2C);
    spi_display_data_send(spi, data, 2);
  //   xSemaphoreGive(priv_mtx);
  // } 
}

void spi_display_draw_circle(spi_device_handle_t spi, uint16_t x0, uint16_t y0, int r, uint16_t color)
{
  int f = 1 - r;
  int ddF_x = 1;
  int ddF_y = -2 * r;
  int x = 0;
  int y = r;
  // if (priv_mtx == NULL) ESP_LOGI("SPI DISPLAY", "SEM NULL");

  // if (xSemaphoreTake(priv_mtx, pdMS_TO_TICKS(2000)) != pdTRUE)
  // {
  //   return;
  // }

  // ESP_LOGI("SPI", "TAKE MTX");

  spi_display_draw_pixel(spi, x0, y0 + r, color);
  spi_display_draw_pixel(spi, x0, y0 - r, color);
  spi_display_draw_pixel(spi, x0 + r, y0, color);
  spi_display_draw_pixel(spi, x0 - r, y0, color);
  while (x < y)
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
    spi_display_draw_pixel(spi, x0 + x, y0 + y, color);
    spi_display_draw_pixel(spi, x0 - x, y0 + y, color);
    spi_display_draw_pixel(spi, x0 + x, y0 - y, color);
    spi_display_draw_pixel(spi, x0 - x, y0 - y, color);
    spi_display_draw_pixel(spi, x0 + y, y0 + x, color);
    spi_display_draw_pixel(spi, x0 - y, y0 + x, color);
    spi_display_draw_pixel(spi, x0 + y, y0 - x, color);
    spi_display_draw_pixel(spi, x0 - y, y0 - x, color);
  }
  // xSemaphoreGive(priv_mtx);

}

//-------------------------------------------------------------------
void spi_display_init(spi_device_handle_t spi, uint16_t w_size, uint16_t h_size)
{
  priv_mtx = xSemaphoreCreateMutex();
  // if (priv_mtx == NULL) ESP_LOGI("SPI DISPLAY", "SEM NULL");
  
  uint8_t data[15];
  // Initialize non-SPI GPIOs
  gpio_set_direction(CONFIG_PIN_NUM_DC, GPIO_MODE_OUTPUT);
  gpio_set_direction(CONFIG_PIN_NUM_RST, GPIO_MODE_OUTPUT);
  gpio_set_direction(CONFIG_PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
  gpio_set_level(CONFIG_PIN_NUM_BCKL, 1);

  spi_display_reset();
  spi_display_cmd_send(spi, SPI_DISPLAY_RESET);

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  for (int i = 0; i < SPI_INIT_DATA_CNT; ++i)
  {
    spi_display_cmd_send(spi, _spi_init_data_buf[i].cmd);
    spi_display_data_send(spi, _spi_init_data_buf[i].data, _spi_init_data_buf[i].lenght);
  }

  spi_display_cmd_send(spi, 0x11);
  spi_display_cmd_send(spi, 0x29);
}