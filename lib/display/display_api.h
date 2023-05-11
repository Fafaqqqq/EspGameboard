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
#define TFT9341_BLUE    0x001F
#define TFT9341_RED     0xF800
#define TFT9341_GREEN   0x07E0
#define TFT9341_CYAN    0x07FF
#define TFT9341_MAGENTA 0xF81F
#define TFT9341_YELLOW  0xFFE0
#define TFT9341_WHITE   0xFFFF

//---------------------------------------------------------------------
void DisplayFill
(
  uint16_t color
);

void DisplayFillRect
(
  uint16_t x1, 
  uint16_t y1, 
  uint16_t x2, 
  uint16_t y2, 
  uint16_t color
);

void DisplayDrawPixel
(
  int x, 
  int y, 
  uint16_t color
);

void DisplayDrawLine
(
  uint16_t color,
  uint16_t x1,
  uint16_t y1,
  uint16_t x2,
  uint16_t y2
);

void DisplayDrawRect
(
  uint16_t color, 
  uint16_t x1,
  uint16_t y1,
  uint16_t x2, 
  uint16_t y2
);

void DisplayDrawCircle
(
  int      radius,
  uint16_t x0,
  uint16_t y0,
  uint16_t color
);

void DisplaySetTextColor
(
  uint16_t color
);

void DisplaySetBackColor
(
  uint16_t color
);

void DisplaySetFont
(
  sFONT *pFonts
);

void DisplayDrawSymbol
(
  uint16_t x,
  uint16_t y,
  uint8_t  symbol
);

void DisplayDrawString
(
  const char* str,
  uint16_t    x,
  uint16_t    y
);

void DisplaySetRotation
(
  uint8_t r
);

void DisplayInit
(
  uint16_t w_size,
  uint16_t h_size
);

//---------------------------------------------------------------------
#endif /* MAIN_SPI_ILI9341_H_ */
