#ifndef SPI_DISPLAY_H
#define SPI_DISPLAY_H

#include <driver/spi_master.h>
#include <driver/gpio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

enum {
  DISPLAY_SIZE_X = 320,
  DISPLAY_SIZE_Y = 240,
};

typedef enum {
  SPI_DISPLAY_RESET = 0x1,

} spi_dev_controller_cmd_t;

typedef struct {

  spi_device_interface_config_t config_device;
  spi_device_handle_t handle_device;

} spi_display_t;

void spi_display_fill(spi_device_handle_t spi, uint16_t color);
void spi_display_draw_circle(spi_device_handle_t spi, uint16_t x0, uint16_t y0, int r, uint16_t color);
void spi_display_init(spi_device_handle_t spi, uint16_t w_size, uint16_t h_size);

#endif