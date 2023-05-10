#include "graphics.h"
#include "spi.h"

#include <driver/spi_master.h>
#include <driver/gpio.h>

#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/FreeRTOS.h"

#include "esp_log.h"

#include <string.h>

static uint32_t DISPLAY_SIZE_Y = 240;
static uint32_t DISPLAY_SIZE_X = 320;
 
static SemaphoreHandle_t sem_lock = NULL;
static SemaphoreHandle_t sem_sign = NULL;

static spi_device_handle_t device_handle = NULL;

static void spi_pre_transfer_cb(spi_transaction_t* spi_trans)
{
  int dc_val = (int)spi_trans->user;
  gpio_set_level(CONFIG_PIN_NUM_DC, dc_val);
}

int graphics_init() 
{
  esp_err_t ret = 0;

  sem_lock = xSemaphoreCreateMutex();
  sem_sign = xSemaphoreCreateBinary();

  spi_bus_config_t spi_cfg = {
    .mosi_io_num = CONFIG_PIN_NUM_MOSI,
    .miso_io_num = -1,
    .sclk_io_num = CONFIG_PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 240 * 320 * 2 + 8,
    .flags = 0
  };

  spi_device_interface_config_t spi_dev_cfg = {
    .clock_speed_hz = 80*1000*1000,     //Clock out at 10 MHz
    .mode = 0,                          //SPI mode 0
    .spics_io_num = CONFIG_PIN_NUM_CS,  //CS pin
    .queue_size = 7,                    //We want to be able to queue 7 transactions at a time
    .pre_cb = &spi_pre_transfer_cb,     //Specify pre-transfer callback to handle D/C line
  };

  ret = spi_init(&spi_cfg, &spi_dev_cfg, &device_handle);
  // ESP_LOGI("GRAPHICS", "spi_init() = %d", ret);
  assert(0 == ret && device_handle != NULL);

  return ret;
}

static void spi_write_data(spi_device_handle_t spi, uint8_t *buff, size_t buff_size)
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

static void spi_set_wind(spi_device_handle_t spi, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  // column address set
  spi_cmd_send(spi, 0x2A); // CASET
  {
    uint8_t data[] = {(x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF};
    spi_write_data(spi, data, sizeof(data));
  }
  // row address set
  spi_cmd_send(spi, 0x2B); // RASET
  {
    uint8_t data[] = {(y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF};
    spi_write_data(spi, data, sizeof(data));
  }
  // write to RAM
  spi_cmd_send(spi, 0x2C); // RAMWR
}

void spi_set_pixel(spi_device_handle_t spi, int x, int y, uint16_t color)
{
  uint8_t data[2];
  if ((x < 0) || (y < 0) || (x >= DISPLAY_SIZE_Y) || (y >= DISPLAY_SIZE_X))
    return;
  data[0] = color >> 8;
  data[1] = color & 0xFF;
  // if (xSemaphoreTake(priv_mtx, pdMS_TO_TICKS(100)) == pdTRUE)
  // {
    spi_set_wind(spi, x, y, x, y);
    // spi_cmd_send(spi, 0x2C);
    spi_data_send(spi, data, 2);
  //   xSemaphoreGive(priv_mtx);
  // } 
}

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

void spi_fill(uint16_t color)
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
    spi_display_send_hor_block(device_handle, i * 16, blck);
    spi_display_block_finish(device_handle);
  }

  heap_caps_free(blck);
}

void graphics_fill_disp(uint16_t color)
{
  assert(device_handle);

  uint16_t* half_display = (uint16_t*)heap_caps_malloc(DISPLAY_SIZE_X * DISPLAY_SIZE_Y / 2 * sizeof(uint16_t), MALLOC_CAP_DMA);
  
  memset(half_display, color, DISPLAY_SIZE_X * DISPLAY_SIZE_Y / 2 * sizeof(uint16_t));

  for (uint32_t i = 0; i < 10; i++) {
    for (uint32_t j = 0; j < 10; j++) {
      half_display[j + i * DISPLAY_SIZE_X] = 0;
    }
  }
  
  spi_send_pixels(device_handle, 0, 0, DISPLAY_SIZE_X, DISPLAY_SIZE_Y / 2, (uint8_t*)half_display);
  spi_send_pixels(device_handle, 0, DISPLAY_SIZE_Y / 2, DISPLAY_SIZE_X, DISPLAY_SIZE_Y / 2, (uint8_t*)half_display);
  
  // for (uint32_t i = 0; i < DISPLAY_SIZE_Y; i++)
  // {
  //   for (uint32_t j = 0; j < DISPLAY_SIZE_X; j++)
  //   {
  //     spi_set_pixel(device_handle, j, i, color);
  //   }
  // }
}