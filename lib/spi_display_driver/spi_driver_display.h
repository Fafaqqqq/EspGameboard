#ifndef SPI_DISPLAY_H
#define SPI_DISPLAY_H

#include <driver/spi_master.h>
#include <driver/gpio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef enum spi_dev_controller_cmd {
  SPI_DISPLAY_CMD_RESET                = 0x1,
  SPI_DISPLAY_CMD_SLEEP_OUT            = 0x11,
  SPI_DISPLAY_CMD_ON                   = 0x29,
  SPI_DISPLAY_CMD_COL_ADDR             = 0x2A,
  SPI_DISPLAY_CMD_PAGE_ADDR            = 0x2B,
  SPI_DISPLAY_CMD_MEM_WR               = 0x2C,
  SPI_DISPLAY_CMD_MEM_ACCESS           = 0x36,
  SPI_DISPLAY_CMD_PIX_FMT              = 0x3A,
  SPI_DISPLAY_CMD_FPS_CTRL             = 0xB1,
  SPI_DISPLAY_CMD_FUNC_CTRL            = 0xB6,
  SPI_DISPLAY_CMD_3G                   = 0xF2,
  SPI_DISPLAY_CMD_POWER_CTRL_A         = 0xCB,
  SPI_DISPLAY_CMD_POWER_CTRL_B         = 0xCF,
  SPI_DISPLAY_CMD_TIME_CTRL_A          = 0xE8,
  SPI_DISPLAY_CMD_TIME_CTRL_B          = 0xEA,
  SPI_DISPLAY_CMD_SEQUENCE_CTRL_A      = 0xED,
  SPI_DISPLAY_CMD_RATIO_CTRL           = 0xF7,
  SPI_DISPLAY_CMD_POWER_CTRL_1         = 0xC0,
  SPI_DISPLAY_CMD_POWER_CTRL_2         = 0xC1,
  SPI_DISPLAY_CMD_VCOM_CTRL_1          = 0xC5,
  SPI_DISPLAY_CMD_VCOM_CTRL_2          = 0xC7,
  SPI_DISPLAY_CMD_POS_GAMMA_CORRECTION = 0xE0,
  SPI_DISPLAY_CMD_NEG_GAMMA_CORRECTION = 0xE1,
  SPI_DISPLAY_CMD_ENTRY_MODE           = 0xB7,
} spi_dev_controller_cmd_t;

typedef spi_device_handle_t spi_display_handle_t;
int spi_driver_display_init(const spi_bus_config_t*              spi_bus_cfg,

                            const spi_device_interface_config_t* spi_device_cfg,
                                  spi_display_handle_t*          spi_display_h);

void spi_driver_display_reset(void);
void spi_driver_display_cmd_send(spi_display_handle_t spi_display_h, uint8_t cmd);
void spi_driver_display_data_send(spi_display_handle_t spi_display_h, const uint8_t *data, int len);
void spi_driver_display_write_data(spi_display_handle_t spi_display_h, uint8_t *buff, size_t buff_size);
void spi_driver_display_transaction_finish(spi_display_handle_t spi_display_h, uint32_t transaction_cnt);
void spi_driver_send_image(spi_display_handle_t spi_display_h, uint32_t size_x, uint32_t size_y, uint16_t* buf_img);
void spi_driver_send_block(spi_display_handle_t spi_display_h, uint32_t x, uint32_t y, 
                             uint32_t size_x, uint32_t size_y, uint16_t* buf_img);
#endif