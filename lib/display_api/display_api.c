#include "display_api.h"
#include "spi_driver_display.h"

#include <string.h>

static spi_display_handle_t display_handle = NULL;

typedef enum display_sizes
{
  DISPLAY_SIZE_X = 320,
  DISPLAY_SIZE_Y = 240,
} display_sizes_t;

static void spi_pre_transfer_callback(spi_transaction_t* spi_trans)
{
  int dc = (int)spi_trans->user;
  gpio_set_level(CONFIG_PIN_NUM_DC, dc);
}

void display_init()
{
  esp_err_t ret = 0;

  spi_bus_config_t spi_cfg = {
    .mosi_io_num = CONFIG_PIN_NUM_MOSI,
    .miso_io_num = -1,
    .sclk_io_num = CONFIG_PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 16*320*2+8,
    .flags = 0
  };

  spi_device_interface_config_t spi_dev_cfg = {
    .clock_speed_hz = 10*1000*1000,           //Clock out at 10 MHz
    .mode = 0,                                //SPI mode 0
    .spics_io_num = CONFIG_PIN_NUM_CS,               //CS pin
    .queue_size = 7,                          //We want to be able to queue 7 transactions at a time
    .pre_cb = &spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
  };

  ret = spi_driver_display_init(&spi_cfg, &spi_dev_cfg, &display_handle);
  assert(0 == ret && display_handle != NULL);

}

static void display_send_x_block(spi_display_handle_t display_h, int ypos, uint16_t *data)
{
  static spi_transaction_t transactions[6];
  esp_err_t ret;

  int x;
  for (x = 0; x < 6; x++)
  {
    memset(&transactions[x], 0, sizeof(spi_transaction_t));
    if ((x & 1) == 0)
    {
      // Even transfers are commands
      transactions[x].length = 8;
      transactions[x].user = (void *)0;
    }
    else
    {
      // Odd transfers are data
      transactions[x].length = 8 * 4;
      transactions[x].user = (void *)1;
    }
    transactions[x].flags = SPI_TRANS_USE_TXDATA;
  }

  transactions[0].tx_data[0] = 0x2A;                    // Column Address Set
  transactions[1].tx_data[0] = 0;                       // Start Col High
  transactions[1].tx_data[1] = 0;                       // Start Col Low
  transactions[1].tx_data[2] = (DISPLAY_SIZE_X) >> 8;   // End Col High
  transactions[1].tx_data[3] = (DISPLAY_SIZE_X) & 0xff; // End Col Low
  transactions[2].tx_data[0] = 0x2B;                    // Page address set
  transactions[3].tx_data[0] = ypos >> 8;               // Start page high
  transactions[3].tx_data[1] = ypos & 0xff;             // start page low
  transactions[3].tx_data[2] = (ypos + 16) >> 8;        // end page high
  transactions[3].tx_data[3] = (ypos + 16) & 0xff;      // end page low
  transactions[4].tx_data[0] = 0x2C;                    // memory write
  transactions[5].tx_buffer = data;                     // finally send the line data
  transactions[5].length = DISPLAY_SIZE_X * 2 * 8 * 16; // Data length, in bits
  transactions[5].flags = 0;                            // undo SPI_TRANS_USE_TXDATA flag
  for (x = 0; x < 6; x++)
  {
    ret = spi_device_queue_trans((spi_device_handle_t)display_h, &transactions[x], portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}


void display_fill_color(uint16_t color)
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
    display_send_x_block(display_handle, i * 16, blck);
    spi_driver_display_transaction_finish(display_handle, 6);
  }

  heap_caps_free(blck);
}

void display_draw_circle(const point_t* center, uint32_t radius, uint16_t color)
{
  
}