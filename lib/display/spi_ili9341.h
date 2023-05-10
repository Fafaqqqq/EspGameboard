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
#define swap(a,b) {int16_t t=a;a=b;b=t;}
//---------------------------------------------------------------------
#define TFT9341_MADCTL_MY  0x80
#define TFT9341_MADCTL_MX  0x40
#define TFT9341_MADCTL_MV  0x20
#define TFT9341_MADCTL_ML  0x10
#define TFT9341_MADCTL_RGB 0x00
#define TFT9341_MADCTL_BGR 0x08
#define TFT9341_MADCTL_MH  0x04
#define TFT9341_BLACK   0x0000
#define TFT9341_BLUE    0x001F
#define TFT9341_RED     0xF800
#define TFT9341_GREEN   0x07E0
#define TFT9341_CYAN    0x07FF
#define TFT9341_MAGENTA 0xF81F
#define TFT9341_YELLOW  0xFFE0
#define TFT9341_WHITE   0xFFFF
//---------------------------------------------------------------------
void TFT9341_FillScreen(spi_device_handle_t spi, uint16_t color);
void TFT9341_FillRect(spi_device_handle_t spi, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void TFT9341_DrawPixel(spi_device_handle_t spi, int x, int y, uint16_t color);
void TFT9341_DrawLine(spi_device_handle_t spi, uint16_t color, uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2);
void TFT9341_DrawRect(spi_device_handle_t spi, uint16_t color, uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2);
void TFT9341_DrawCircle(spi_device_handle_t spi, uint16_t x0, uint16_t y0, int r, uint16_t color);
void TFT9341_SetTextColor(uint16_t color);
void TFT9341_SetBackColor(uint16_t color);
void TFT9341_SetFont(sFONT *pFonts);
void TFT9341_DrawChar(spi_device_handle_t spi, uint16_t x, uint16_t y, uint8_t c);
void TFT9341_String(spi_device_handle_t spi, uint16_t x,uint16_t y, char *str);
void TFT9341_SetRotation(spi_device_handle_t spi, uint8_t r);
void TFT9341_ini(spi_device_handle_t spi, uint16_t w_size, uint16_t h_size);
//---------------------------------------------------------------------
#endif /* MAIN_SPI_ILI9341_H_ */
