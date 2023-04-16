#ifndef DISPLAY_API_H
#define DISPLAY_API_H

#include <stdint.h>

typedef struct point
{
  uint32_t x;
  uint32_t y;
} point_t;

void display_init();
void display_draw_circle(const point_t* center, uint32_t radius, uint16_t color);
void display_fill_color(uint16_t color);
void display_fill_data(uint16_t* data);

#endif