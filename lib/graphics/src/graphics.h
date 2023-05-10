#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

int graphics_init();

void graphics_fill_disp
(
  uint16_t color
);

void spi_fill
(
  uint16_t color
);

#endif