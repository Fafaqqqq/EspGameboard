#ifndef SPI_INIT_H
#define SPI_INIT_H

#include <driver/spi_master.h>
#include <driver/gpio.h>

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

int spi_init
(
  const spi_bus_config_t*              spi_bus_cfg,
  const spi_device_interface_config_t* spi_device_cfg,
  spi_device_handle_t*                 spi_device_h
);

void spi_cmd_send
(
  spi_device_handle_t spi_device_h, 
  uint8_t              cmd
);

void spi_data_send
(
  spi_device_handle_t  spi_device_h, 
  const uint8_t*       data, 
  int32_t              length
);

void spi_send_image
(
  spi_device_handle_t spi_device_h, 
  uint32_t            size_x, 
  uint32_t            size_y, 
  const uint16_t*     buf
);

void spi_send_pixels
(
  spi_device_handle_t spi_device_h,
  uint32_t            pos_x,
  uint32_t            pos_y,
  uint32_t            size_x, 
  uint32_t            size_y, 
  const uint8_t*      buf
);

void spi_reset(void);

#endif