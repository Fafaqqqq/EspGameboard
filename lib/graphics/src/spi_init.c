#include "spi.h"

#include "esp_log.h"
#include "freertos/task.h"

#define SPI_MAX_INIT_DATA_LENGTH 0x000F
#define SPI_INIT_DATA_CNT        0x0016
#define SPI_MAX_DATA_CHUNK_SIZE  0x8000

static const char* _tag = "spi_init";

struct spi_init_table_t 
{
  uint8_t data[SPI_MAX_INIT_DATA_LENGTH];
  uint8_t lenght;
  spi_dev_controller_cmd_t cmd;
};

static struct spi_init_table_t spi_init_table[SPI_INIT_DATA_CNT] = {
    {.data = {0x39, 0x2C, 0x00, 0x34, 0x02},
     .lenght = 5,
     .cmd = SPI_DISPLAY_CMD_POWER_CTRL_A},
    {.data = {0x00, 0x83, 0x30},
     .lenght = 3,
     .cmd = SPI_DISPLAY_CMD_POWER_CTRL_B},
    {.data = {0x85, 0x01, 0x79},
     .lenght = 3,
     .cmd = SPI_DISPLAY_CMD_TIME_CTRL_A},
    {.data = {0x0, 0x0},
     .lenght = 2,
     .cmd = SPI_DISPLAY_CMD_TIME_CTRL_B},
    {.data = {0x64, 0x03, 0x12, 0x81},
     .lenght = 4,
     .cmd = SPI_DISPLAY_CMD_SEQUENCE_CTRL_A},
    {.data = {0x20},
     .lenght = 1,
     .cmd = SPI_DISPLAY_CMD_RATIO_CTRL},
    {.data = {0x26},
     .lenght = 1,
     .cmd = SPI_DISPLAY_CMD_POWER_CTRL_1},
    {.data = {0x11},
     .lenght = 1,
     .cmd = SPI_DISPLAY_CMD_POWER_CTRL_2},
    {.data = {0x35, 0x3E},
     .lenght = 2,
     .cmd = SPI_DISPLAY_CMD_VCOM_CTRL_1},
    {.data = {0xBE},
     .lenght = 1,
     .cmd = SPI_DISPLAY_CMD_VCOM_CTRL_2},
    {.data = {0x28},
     .lenght = 1,
     .cmd = SPI_DISPLAY_CMD_MEM_ACCESS},
    {.data = {0x55},
     .lenght = 1,
     .cmd = SPI_DISPLAY_CMD_PIX_FMT},
    {.data = {0x00, 0x1B},
     .lenght = 2,
     .cmd = SPI_DISPLAY_CMD_FPS_CTRL},
    {.data = {0x08, 0x82, 0x27},
     .lenght = 3,
     .cmd = SPI_DISPLAY_CMD_FUNC_CTRL,},
    {.data = {0x08},
     .lenght = 1,
     .cmd = SPI_DISPLAY_CMD_3G},
    {.data = {0x01},
     .lenght = 1,
     .cmd = 0x25},
    {.data = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00},
     .lenght = 15,
     .cmd = SPI_DISPLAY_CMD_POS_GAMMA_CORRECTION},
    {.data = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F},
     .lenght = 15,
     .cmd = SPI_DISPLAY_CMD_NEG_GAMMA_CORRECTION},
    {.data = {0x00, 0x00, 0x00, 0xEF},
     .lenght = 4,
     .cmd = SPI_DISPLAY_CMD_COL_ADDR},
    {.data = {0x00, 0x00, 0x01, 0x3F},
     .lenght = 4,
     .cmd = SPI_DISPLAY_CMD_PAGE_ADDR},
    {.data = {0x07},
     .lenght = 1,
     .cmd = SPI_DISPLAY_CMD_ENTRY_MODE}};

int spi_init
(
  const spi_bus_config_t*              spi_bus_cfg,
  const spi_device_interface_config_t* spi_device_cfg,
  spi_device_handle_t*                 spi_device_h
)
{
  esp_err_t ret = 0;

  if (NULL != *spi_device_h)
  {
    ESP_LOGE(_tag, "ERROR!!!");
    return -__LINE__;
  }

  ret = spi_bus_initialize(VSPI_HOST, spi_bus_cfg, SPI_DMA_CH_AUTO);
  ESP_LOGI(_tag, "spi bus initialize: %d", ret);
  if (0 != ret)
  {
    return ret;
  }

  ret = spi_bus_add_device(VSPI_HOST, spi_device_cfg, spi_device_h);
  ESP_LOGI(_tag, "spi bus add device: %d", ret);
  if (0 != ret)
  {
    return ret;
  }

  // Initialize non-SPI GPIOs
  gpio_set_direction(CONFIG_PIN_NUM_DC, GPIO_MODE_OUTPUT);
  gpio_set_direction(CONFIG_PIN_NUM_RST, GPIO_MODE_OUTPUT);
  gpio_set_direction(CONFIG_PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
  gpio_set_level(CONFIG_PIN_NUM_BCKL, 1);

  spi_reset();
  spi_cmd_send(*spi_device_h, SPI_DISPLAY_CMD_RESET);

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  for (int i = 0; i < SPI_INIT_DATA_CNT; ++i)
  {
    spi_cmd_send(*spi_device_h, spi_init_table[i].cmd);
    spi_data_send(*spi_device_h, spi_init_table[i].data, spi_init_table[i].lenght);
  }

  spi_cmd_send(*spi_device_h, SPI_DISPLAY_CMD_SLEEP_OUT);
  spi_cmd_send(*spi_device_h, SPI_DISPLAY_CMD_ON);

  return ret;
}