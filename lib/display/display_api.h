#ifndef MAIN_SPI_ILI9341_H_
#define MAIN_SPI_ILI9341_H_
//---------------------------------------------------------------------
#include <unistd.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "sdkconfig.h"

#include "fonts.h"
//---------------------------------------------------------------------
extern uint16_t TFT9341_WIDTH;
extern uint16_t TFT9341_HEIGHT;


//---------------------------------------------------------------------
#define TFT9341_MADCTL_MY  0x80
#define TFT9341_MADCTL_MX  0x40
#define TFT9341_MADCTL_MV  0x20
#define TFT9341_MADCTL_ML  0x10
#define TFT9341_MADCTL_RGB 0x00
#define TFT9341_MADCTL_BGR 0x08
#define TFT9341_MADCTL_MH  0x04

//---------------------------------------------------------------------
#define TFT9341_BLACK   0x0000
#define TFT9341_RED     0x001F
#define TFT9341_BLUE    0xF800
#define TFT9341_GREEN   0x07E0
#define TFT9341_CYAN    0x07FF
#define TFT9341_MAGENTA 0xF81F
#define TFT9341_YELLOW  0xFFE0
#define TFT9341_WHITE   0xFFFF

//---------------------------------------------------------------------
void display_fill
(
  uint16_t color
);

void display_fill_rect
(
  uint16_t x1, 
  uint16_t y1, 
  uint16_t x2, 
  uint16_t y2, 
  uint16_t color
);

void display_fill_circle
(
  int      radius,
  uint16_t x0,
  uint16_t y0,
  uint16_t color
);

void display_draw_pixel
(
  int x, 
  int y, 
  uint16_t color
);

void display_draw_line
(
  uint16_t color,
  uint16_t x1,
  uint16_t y1,
  uint16_t x2,
  uint16_t y2
);

void display_draw_rect
(
  uint16_t color, 
  uint16_t x1,
  uint16_t y1,
  uint16_t x2, 
  uint16_t y2
);

void display_draw_circle
(
  int      radius,
  uint16_t x0,
  uint16_t y0,
  uint16_t color
);

void display_set_text_color
(
  uint16_t color
);

void display_set_back_color
(
  uint16_t color
);

void display_set_font
(
  sFONT *pFonts
);

void display_draw_symbol
(
  uint16_t x,
  uint16_t y,
  uint8_t  symbol
);

void display_draw_string
(
  const char* str,
  uint16_t    x,
  uint16_t    y
);

void display_set_rotation
(
  uint8_t r
);

void display_init
(
  uint16_t w_size,
  uint16_t h_size
);

//---------------------------------------------------------------------
#endif /* MAIN_SPI_ILI9341_H_ */
