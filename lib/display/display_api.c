#include "display_api.h"
#include "esp_log.h"

#define swap(a, b) \
  {                \
    int16_t t = a; \
    a = b;         \
    b = t;         \
  }

//-------------------------------------------------------------------
typedef struct
{
  uint16_t TextColor;
  uint16_t BackColor;
  sFONT*   pFont;
} spi_text_t;

static const char *TAG = "grapics_api";

uint16_t TFT9341_WIDTH;
uint16_t TFT9341_HEIGHT;

static spi_text_t spi_text;
static spi_device_handle_t spi_dev_h = NULL;

static void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
  int dc = (int)t->user;
  gpio_set_level(CONFIG_PIN_NUM_DC, dc);
}

//-------------------------------------------------------------------
static void spi_cmd
(  
  const uint8_t cmd
)
{
  esp_err_t ret;
  spi_transaction_t transaction;
  memset(&transaction, 0, sizeof(transaction));                         

  transaction.length    = 8;
  transaction.tx_buffer = &cmd;
  transaction.user      = (void *)0;

  ret = spi_device_polling_transmit(spi_dev_h, &transaction);
  assert(ret == ESP_OK);
}

//-------------------------------------------------------------------
static void spi_data
(
  const uint8_t *data,
  int len
)
{
  esp_err_t ret;
  spi_transaction_t transaction;
  memset(&transaction, 0, sizeof(transaction));                         
  
  if (len == 0)
    return;                                                   
    
  transaction.length    = len * 8;                            
  transaction.tx_buffer = data;                               
  transaction.user      = (void *)1;    

  ret = spi_device_polling_transmit(spi_dev_h, &transaction); 
  assert(ret == ESP_OK);                                      
}

//-------------------------------------------------------------------
static void spi_display_reset()
{
  gpio_set_level(CONFIG_PIN_NUM_RST, 0);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  gpio_set_level(CONFIG_PIN_NUM_RST, 1);
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

//-------------------------------------------------------------------
static void spi_send_horizontal_blocks
(
  int ypos,
  uint16_t *data
)
{
  esp_err_t ret;
  static spi_transaction_t transaction[6];
  
  for (int i = 0; i < 6; i++)
  {
    memset(&transaction[i], 0, sizeof(spi_transaction_t));

    if ((i & 1) == 0)
    {
      transaction[i].length = 8;
      transaction[i].user = (void *)0;
    }
    else
    {
      transaction[i].length = 8 * 4;
      transaction[i].user = (void *)1;
    }

    transaction[i].flags = SPI_TRANS_USE_TXDATA;
  }

  transaction[0].tx_data[0] = 0x2A;

  transaction[1].tx_data[0] = 0;
  transaction[1].tx_data[1] = 0;
  transaction[1].tx_data[2] = (TFT9341_WIDTH) >> 8;
  transaction[1].tx_data[3] = (TFT9341_WIDTH) & 0xff;

  transaction[2].tx_data[0] = 0x2B;

  transaction[3].tx_data[0] = ypos >> 8;
  transaction[3].tx_data[1] = ypos & 0xff;
  transaction[3].tx_data[2] = (ypos + 16) >> 8;
  transaction[3].tx_data[3] = (ypos + 16) & 0xff;

  transaction[4].tx_data[0] = 0x2C;

  transaction[5].tx_buffer = data;
  transaction[5].length    = TFT9341_WIDTH * 2 * 8 * 16;
  transaction[5].flags     = 0;

  for (int i = 0; i < 6; i++)
  {
    ret = spi_device_queue_trans(spi_dev_h, &transaction[i], portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}

//-------------------------------------------------------------------
static void spi_send_blocks
(
  int x1,
  int y1,
  int x2,
  int y2,
  uint16_t *data
)
{
  esp_err_t ret;
  static spi_transaction_t transaction[6];

  for (int i = 0; i < 6; i++)
  {
    memset(&transaction[i], 0, sizeof(spi_transaction_t));
    if ((i & 1) == 0)
    {
      transaction[i].length = 8;
      transaction[i].user = (void *)0;
    }
    else
    {
      transaction[i].length = 8 * 4;
      transaction[i].user = (void *)1;
    }

    transaction[i].flags = SPI_TRANS_USE_TXDATA;
  }

  transaction[0].tx_data[0] = 0x2A;

  transaction[1].tx_data[0] = (x1 >> 8) & 0xFF;
  transaction[1].tx_data[1] = x1 & 0xFF;
  transaction[1].tx_data[2] = (x2 >> 8) & 0xFF;
  transaction[1].tx_data[3] = x2 & 0xFF;

  transaction[2].tx_data[0] = 0x2B;

  transaction[3].tx_data[0] = (y1 >> 8) & 0xFF;
  transaction[3].tx_data[1] = y1 & 0xFF;
  transaction[3].tx_data[2] = (y2 >> 8) & 0xFF;
  transaction[3].tx_data[3] = y2 & 0xFF;

  transaction[4].tx_data[0] = 0x2C;

  transaction[5].tx_buffer = data;
  transaction[5].length    = (x2 - x1 + 1) * (y2 - y1 + 1) * 2 * 8;
  transaction[5].flags     = 0;

  for (int i = 0; i < 6; i++)
  {
    ret = spi_device_queue_trans(spi_dev_h, &transaction[i], portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}

//-------------------------------------------------------------------
static void spi_send_block_finish()
{
  esp_err_t ret;
  spi_transaction_t *rtrans;
  
  for (int i = 0; i < 6; i++)
  {
    ret = spi_device_get_trans_result(spi_dev_h, &rtrans, portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}

//-------------------------------------------------------------------
void DisplayFill
(
  uint16_t color
)
{
  uint16_t *buf = heap_caps_malloc(TFT9341_WIDTH * 16 * sizeof(uint16_t), MALLOC_CAP_DMA);;
  
  color = color << 8 | color >> 8;
  
  for (int y = 0; y < 16; y++)
  {
    for (int x = 0; x < TFT9341_WIDTH; x++)
    {
      buf[y * TFT9341_WIDTH + x] = color;
    }
  }

  for (int i = 0; i < TFT9341_HEIGHT / 16; i++)
  {
    spi_send_horizontal_blocks(i * 16, buf);
    spi_send_block_finish(spi_dev_h);
  }

  heap_caps_free(buf);
}
//-------------------------------------------------------------------
void DisplayFillRect
(
  uint16_t x1, 
  uint16_t y1, 
  uint16_t x2, 
  uint16_t y2, 
  uint16_t color
)
{
  if ((x1 >= TFT9341_WIDTH) || (y1 >= TFT9341_HEIGHT) || (x2 >= TFT9341_WIDTH) || (y2 >= TFT9341_HEIGHT))
    return;

  if (x1 > x2)
    swap(x1, x2);

  if (y1 > y2)
    swap(y1, y2);

  color = color << 8 | color >> 8;

  uint16_t *blck;
  uint16_t  size_y_block;
  uint32_t  size_block;
  uint16_t  size_x   = (x2 - x1 + 1);
  uint16_t  size_y   = (y2 - y1 + 1);
  uint32_t  size     = size_x * size_y;
  uint32_t  size_max = TFT9341_WIDTH * 16;

  blck = heap_caps_malloc(size_max * sizeof(uint16_t), MALLOC_CAP_DMA);

  while (1)
  {
    if (size > size_max)
    {
      size_block   = size_max - (size_max % size_x); // ������� �������, ����� ��������� ����������� �����
      size        -= size_block;
      size_y_block = size_max / size_x;

      for (int y = 0; y < size_y_block; y++)
      {
        for (int x = 0; x < size_x; x++)
        {
          blck[y * size_x + x] = color;
        }
      }

      spi_send_blocks(x1, y1, x2, y1 + size_y_block - 1, blck);
      spi_send_block_finish(spi_dev_h);
      y1 += size_y_block;
    }
    else
    {
      size_y_block = size / size_x;

      for (int y = 0; y < size_y_block; y++)
      {
        for (int x = 0; x < size_x; x++)
        {
          blck[y * size_x + x] = color;
        }
      }

      spi_send_blocks(x1, y1, x2, y1 + size_y_block - 1, blck);
      spi_send_block_finish(spi_dev_h);
      break;
    }
  }
  heap_caps_free(blck);
}

//-------------------------------------------------------------------
static void spi_write_data
(
  uint8_t *buff,
  size_t   buff_size
)
{
  esp_err_t ret;
  spi_transaction_t transaction;

  while (buff_size > 0)
  {
    uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;

    if (chunk_size == 0)
      return;
    memset(&transaction, 0, sizeof(transaction)); 

    transaction.length    = chunk_size * 8;
    transaction.tx_buffer = buff;
    transaction.user      = (void *)1;

    ret = spi_device_polling_transmit(spi_dev_h, &transaction);
    assert(ret == ESP_OK);

    buff += chunk_size;
    buff_size -= chunk_size;
  }
}
//-------------------------------------------------------------------
static void spi_set_addr_wind
(
  uint16_t x0,
  uint16_t y0,
  uint16_t x1,
  uint16_t y1
)
{
  spi_cmd(0x2A); // CASET
  {
    uint8_t data[] = 
    {
      (x0 >> 8) & 0xFF,
      x0 & 0xFF,
      (x1 >> 8) & 0xFF,
      x1 & 0xFF
    };
    spi_write_data(data, sizeof(data));
  }

  spi_cmd(0x2B); // RASET
  {
    uint8_t data[] = 
    {
      (y0 >> 8) & 0xFF, 
      y0 & 0xFF, 
      (y1 >> 8) & 0xFF,
       y1 & 0xFF
    };
    spi_write_data(data, sizeof(data));
  }

  spi_cmd(0x2C); // RAMWR
}

//-------------------------------------------------------------------
void DisplayDrawPixel
(
  int x, 
  int y, 
  uint16_t color
)
{
  if ((x < 0) || (y < 0) || (x >= TFT9341_WIDTH) || (y >= TFT9341_HEIGHT))
    return;

  uint8_t data[2] =
  {
    color >> 8,
    color & 0xFF
  };
  
  spi_set_addr_wind(x, y, x, y);
  spi_cmd(0x2C);
  spi_data(data, 2);
}

//------------------------------------------------------------------
void DisplayDrawLine
(
  uint16_t color,
  uint16_t x1,
  uint16_t y1,
  uint16_t x2,
  uint16_t y2
)
{
  int steep = abs(y2 - y1) > abs(x2 - x1);

  if (steep)
  {
    swap(x1, y1);
    swap(x2, y2);
  }

  if (x1 > x2)
  {
    swap(x1, x2);
    swap(y1, y2);
  }

  int dx = x2 - x1;
  int dy = abs(y2 - y1);
  int err = dx / 2;
  int ystep;

  if (y1 < y2)
    ystep = 1;
  else
    ystep = -1;

  for (; x1 <= x2; x1++)
  {
    if (steep)
      DisplayDrawPixel(y1, x1, color);
    else
      DisplayDrawPixel(x1, y1, color);

    err -= dy;

    if (err < 0)
    {
      y1 += ystep;
      err = dx;
    }
  }
}

//------------------------------------------------------------------
void DisplayDrawRect
(
  uint16_t color, 
  uint16_t x1,
  uint16_t y1,
  uint16_t x2, 
  uint16_t y2
)
{
  DisplayDrawLine(color, x1, y1, x2, y1);
  DisplayDrawLine(color, x2, y1, x2, y2);
  DisplayDrawLine(color, x1, y1, x1, y2);
  DisplayDrawLine(color, x1, y2, x2, y2);
}

//-------------------------------------------------------------------
void DisplayDrawCircle
(
  int      radius,
  uint16_t x0,
  uint16_t y0,
  uint16_t color
)
{
  int x = 0;
  int y = radius;
  int f = 1 - radius;

  int ddF_x = 1;
  int ddF_y = -2 * radius;

  DisplayDrawPixel(x0, y0 + radius, color);
  DisplayDrawPixel(x0, y0 - radius, color);
  DisplayDrawPixel(x0 + radius, y0, color);
  DisplayDrawPixel(x0 - radius, y0, color);

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
    f     += ddF_x;

    DisplayDrawPixel(x0 + x, y0 + y, color);
    DisplayDrawPixel(x0 - x, y0 + y, color);
    DisplayDrawPixel(x0 + x, y0 - y, color);
    DisplayDrawPixel(x0 - x, y0 - y, color);
    DisplayDrawPixel(x0 + y, y0 + x, color);
    DisplayDrawPixel(x0 - y, y0 + x, color);
    DisplayDrawPixel(x0 + y, y0 - x, color);
    DisplayDrawPixel(x0 - y, y0 - x, color);
  }
}

//-------------------------------------------------------------------
void DisplaySetTextColor
(
  uint16_t color
)
{
  spi_text.TextColor = color;
}

//-------------------------------------------------------------------
void DisplaySetBackColor
(
  uint16_t color
)
{
  spi_text.BackColor = color;
}

//-------------------------------------------------------------------
void DisplaySetFont
(
  sFONT *pFonts
)
{
  spi_text.pFont = pFonts;
}

//-------------------------------------------------------------------
void DisplayDrawSymbol
(
  uint16_t x,
  uint16_t y,
  uint8_t symbol
)
{
  uint8_t *c_t;
  uint8_t *pchar;

  uint32_t line = 0;

  uint16_t height = spi_text.pFont->Height;
  uint16_t width = spi_text.pFont->Width;

  uint8_t offset = 8 * ((width + 7) / 8) - width;
  
  c_t = (uint8_t*)&(spi_text.pFont->table[(symbol - ' ') * spi_text.pFont->Height * ((spi_text.pFont->Width + 7) / 8)]);

  for (uint32_t i = 0; i < height; i++)
  {
    pchar = ((uint8_t *)c_t + (width + 7) / 8 * i);

    switch (((width + 7) / 8))
    {
      case 1:
        line = pchar[0];
        break;
      case 2:
        line = (pchar[0] << 8) | pchar[1];
        break;
      case 3:
      default:
        line = (pchar[0] << 16) | (pchar[1] << 8) | pchar[2];
        break;
    }

    for (uint32_t j = 0; j < width; j++)
    {
      if (line & (1 << (width - j + offset - 1)))
      {
        DisplayDrawPixel((x + j), y, spi_text.TextColor);
      }
      else
      {
        DisplayDrawPixel((x + j), y, spi_text.BackColor);
      }
    }
    y++;
  }
}

//-------------------------------------------------------------------
void DisplayDrawString
(
  const char* str,
  uint16_t    x,
  uint16_t    y
)
{
  while (*str)
  {
    DisplayDrawSymbol(x, y, str[0]);
    x += spi_text.pFont->Width;
    (void)*str++;
  }
}

//-------------------------------------------------------------------
void DisplaySetRotation
(
  uint8_t r
)
{
  uint8_t data[1];
  spi_cmd(0x36);
  switch (r)
  {
    case 0:
      data[0] = 0x48;
      spi_data(data, 1);
      TFT9341_WIDTH = 240;
      TFT9341_HEIGHT = 320;
      break;
    case 1:
      data[0] = 0x28;
      spi_data(data, 1);
      TFT9341_WIDTH = 320;
      TFT9341_HEIGHT = 240;
      break;
    case 2:
      data[0] = 0x88;
      spi_data(data, 1);
      TFT9341_WIDTH = 240;
      TFT9341_HEIGHT = 320;
      break;
    case 3:
      data[0] = 0xE8;
      spi_data(data, 1);
      TFT9341_WIDTH = 320;
      TFT9341_HEIGHT = 240;
      break;
  }
}
//-------------------------------------------------------------------

void DisplayInit
(
  uint16_t w_size,
  uint16_t h_size
)
{
  esp_err_t ret;

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
  ESP_LOGI(TAG, "spi_dev_h bus initialize: %d", ret);

  spi_device_interface_config_t devcfg =
      {
          .clock_speed_hz = 10 * 1000 * 1000,      // Clock out at 10 MHz
          .mode = 0,                               // SPI mode 0
          .spics_io_num = CONFIG_PIN_NUM_CS,       // CS pin
          .queue_size = 7,                         // We want to be able to queue 7 transactions at a time
          .pre_cb = lcd_spi_pre_transfer_callback, // Specify pre-transfer callback to handle D/C line
      };
  ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi_dev_h);
  ESP_LOGI(TAG, "spi_dev_h bus add device: %d", ret);

  uint8_t data[15];
  // Initialize non-SPI GPIOs
  gpio_set_direction(CONFIG_PIN_NUM_DC, GPIO_MODE_OUTPUT);
  gpio_set_direction(CONFIG_PIN_NUM_RST, GPIO_MODE_OUTPUT);
  gpio_set_direction(CONFIG_PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
  gpio_set_level(CONFIG_PIN_NUM_BCKL, 1);

  // Software Reset
  spi_display_reset();

  spi_cmd(0x01);

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Power control A, Vcore=1.6V, DDVDH=5.6V
  data[0] = 0x39;
  data[1] = 0x2C;
  data[2] = 0x00;
  data[3] = 0x34;
  data[4] = 0x02;
  spi_cmd(0xCB);
  spi_data(data, 5);

  // Power contorl B, power control = 0, DC_ENA = 1
  data[0] = 0x00;
  data[1] = 0x83;
  data[2] = 0x30;
  spi_cmd(0xCF);
  spi_data(data, 3);

  // Driver timing control A,
  // non-overlap=default +1
  // EQ=default - 1, CR=default
  // pre-charge=default - 1
  data[0] = 0x85;
  data[1] = 0x01;
  data[2] = 0x79;
  spi_cmd(0xE8);
  spi_data(data, 3);

  // Driver timing control, all=0 unit
  data[0] = 0x00;
  data[1] = 0x00;
  spi_cmd(0xEA);
  spi_data(data, 2);

  // Power on sequence control,
  // cp1 keeps 1 frame, 1st frame enable
  // vcl = 0, ddvdh=3, vgh=1, vgl=2
  // DDVDH_ENH=1
  data[0] = 0x64;
  data[1] = 0x03;
  data[2] = 0x12;
  data[3] = 0x81;
  spi_cmd(0xED);
  spi_data(data, 4);

  // Pump ratio control, DDVDH=2xVCl
  data[0] = 0x20;
  spi_cmd(0xF7);
  spi_data(data, 1);

  // Power control 1, GVDD=4.75V
  data[0] = 0x26;
  spi_cmd(0xC0);
  spi_data(data, 1);

  // Power control 2, DDVDH=VCl*2, VGH=VCl*7, VGL=-VCl*3
  data[0] = 0x11;
  spi_cmd(0xC1);
  spi_data(data, 1);

  // VCOM control 1, VCOMH=4.025V, VCOML=-0.950V
  data[0] = 0x35;
  data[1] = 0x3E;
  spi_cmd(0xC5);
  spi_data(data, 2);

  // VCOM control 2, VCOMH=VMH-2, VCOML=VML-2
  data[0] = 0xBE;
  spi_cmd(0xC7);
  spi_data(data, 1);

  // Memory access contorl, MX=MY=0, MV=1, ML=0, BGR=1, MH=0
  data[0] = 0b1000;
  spi_cmd(0x36);
  spi_data(data, 1);

  // Pixel format, 16bits/pixel for RGB/MCU interface
  data[0] = 0x55;
  spi_cmd(0x3A);
  spi_data(data, 1);

  // Frame rate control, f=fosc, 70Hz fps
  data[0] = 0x00;
  data[1] = 0x1B;
  spi_cmd(0xB1);
  spi_data(data, 2);

  // Display function control
  data[0] = 0x08;
  data[1] = 0x82;
  data[2] = 0x27;
  spi_cmd(0xB6);
  spi_data(data, 3);

  // Enable 3G, disabled
  data[0] = 0x08;
  spi_cmd(0xF2);
  spi_data(data, 1);

  // Gamma set, curve 1
  data[0] = 0x01;
  spi_cmd(0x26);
  spi_data(data, 1);

  // Positive gamma correction
  data[0] = 0x0F;
  data[1] = 0x31;
  data[2] = 0x2B;
  data[3] = 0x0C;
  data[4] = 0x0E;
  data[5] = 0x08;
  data[6] = 0x4E;
  data[7] = 0xF1;
  data[8] = 0x37;
  data[9] = 0x07;
  data[10] = 0x10;
  data[11] = 0x03;
  data[12] = 0x0E;
  data[13] = 0x09;
  data[14] = 0x00;
  spi_cmd(0xE0);
  spi_data(data, 15);

  // Negative gamma correction
  data[0] = 0x00;
  data[1] = 0x0E;
  data[2] = 0x14;
  data[3] = 0x03;
  data[4] = 0x11;
  data[5] = 0x07;
  data[6] = 0x31;
  data[7] = 0xC1;
  data[8] = 0x48;
  data[9] = 0x08;
  data[10] = 0x0F;
  data[11] = 0x0C;
  data[12] = 0x31;
  data[13] = 0x36;
  data[14] = 0x0F;
  spi_cmd(0xE1);
  spi_data(data, 15);

  // Column address set, SC=0, EC=0xEF
  data[0] = 0x00;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0xEF;
  spi_cmd(0x2A);
  spi_data(data, 4);

  // Page address set, SP=0, EP=0x013F
  data[0] = 0x00;
  data[1] = 0x00;
  data[2] = 0x01;
  data[3] = 0x3F;
  spi_cmd(0x2B);
  spi_data(data, 4);

  // Memory write
  spi_cmd(0x2C);

  // Entry mode set, Low vol detect disabled, normal display
  data[0] = 0x07;
  spi_cmd(0xB7);
  spi_data(data, 1);

  // Sleep out
  spi_cmd(0x11);
  // Display on
  spi_cmd(0x29);

  TFT9341_WIDTH = w_size;
  TFT9341_HEIGHT = h_size;
}
//-------------------------------------------------------------------
