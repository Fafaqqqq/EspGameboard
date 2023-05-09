#ifndef DISPLAY_API_H
#define DISPLAY_API_H

#include <stdint.h>

typedef struct pixel
{
  uint32_t x;
  uint32_t y;
} pixel_t;


void display_init();
void display_draw_pixel(uint32_t x, uint32_t y, uint16_t color);
void display_send_buffer();

void display_draw_circle(const pixel_t* center, uint32_t radius, uint16_t color);
void display_fill_color(uint16_t color);
void display_fill_data(const uint16_t* data);

void display_draw_plot(int32_t x, uint16_t color);


void spi_display_fill(uint16_t color);


#endif