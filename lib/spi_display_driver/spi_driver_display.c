#include "spi_driver_display.h"

#include <esp_log.h>
#include <string.h>

#define SPI_MAX_INIT_DATA_LENGTH 0x000F
#define SPI_INIT_DATA_CNT        0x0016
#define SPI_MAX_DATA_CHUNK_SIZE  0x8000

typedef struct
{
  uint8_t data[SPI_MAX_INIT_DATA_LENGTH];
  uint8_t lenght;
  spi_dev_controller_cmd_t cmd;
} spi_data_t;

typedef const char* tag_t;

static tag_t _tag                                       = "spi_driver";
static spi_display_handle_t _spi_display_h              = NULL;
static spi_data_t _spi_init_data_buf[SPI_INIT_DATA_CNT] = {
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

int spi_driver_display_init(const spi_bus_config_t*              spi_bus_cfg,
                            const spi_device_interface_config_t* spi_device_cfg,
                                  spi_display_handle_t*          spi_display_h)
{
  esp_err_t ret = 0;

  if (NULL != spi_display_h)
  {
    return -__LINE__;
  }

  ret = spi_bus_initialize(HSPI_HOST, spi_bus_cfg, SPI_DMA_CH_AUTO);
  ESP_LOGI(_tag, "spi bus initialize: %d", ret);
  if (0 != ret)
  {
    return ret;
  }

  ret = spi_bus_add_device(HSPI_HOST, spi_device_cfg, (spi_device_handle_t*)&_spi_display_h);
  ESP_LOGI(_tag, "spi bus add device: %d", ret);
  if (0 != ret)
  {
    return ret;
  }

  *spi_display_h = _spi_display_h;

  // Initialize non-SPI GPIOs
  gpio_set_direction(CONFIG_PIN_NUM_DC, GPIO_MODE_OUTPUT);
  gpio_set_direction(CONFIG_PIN_NUM_RST, GPIO_MODE_OUTPUT);
  gpio_set_direction(CONFIG_PIN_NUM_BCKL, GPIO_MODE_OUTPUT);
  gpio_set_level(CONFIG_PIN_NUM_BCKL, 1);

  spi_driver_display_reset();
  spi_driver_display_cmd_send(_spi_display_h, SPI_DISPLAY_CMD_RESET);

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  for (int i = 0; i < SPI_INIT_DATA_CNT; ++i)
  {
    spi_driver_display_cmd_send(_spi_display_h, _spi_init_data_buf[i].cmd);
    spi_driver_display_data_send(_spi_display_h, _spi_init_data_buf[i].data, _spi_init_data_buf[i].lenght);
  }

  spi_driver_display_cmd_send(_spi_display_h, SPI_DISPLAY_CMD_SLEEP_OUT);
  spi_driver_display_cmd_send(_spi_display_h, SPI_DISPLAY_CMD_ON);

  return ret;
}

void spi_driver_display_cmd_send(spi_display_handle_t spi_display_h, uint8_t cmd)
{
  assert(spi_display_h == _spi_display_h);

  esp_err_t ret;
  spi_transaction_t spi_trans;

  memset(&spi_trans, 0, sizeof(spi_trans)); // Zero out the transaction

  spi_trans.length = 8;       // Command is 8 bits
  spi_trans.tx_buffer = &cmd; // The data is the cmd itself
  spi_trans.user = (void *)0; // D/C needs to be set to 0

  ret = spi_device_polling_transmit(spi_display_h, &spi_trans); // Transmit!
  assert(ret == ESP_OK);                              // Should have had no issues.
}

void spi_driver_display_data_send(spi_display_handle_t spi_display_h, const uint8_t *data, int len)
{
  assert(spi_display_h == _spi_display_h);

  esp_err_t ret;
  spi_transaction_t t;

  if (len == 0)
    return; // no need to send anything

  memset(&t, 0, sizeof(t)); // Zero out the transaction

  t.length = len * 8;                         // Len is in bytes, transaction length is in bits.
  t.tx_buffer = data;                         // Data
  t.user = (void *)1;                         // D/C needs to be set to 1

  ret = spi_device_polling_transmit(spi_display_h, &t); // Transmit!
  assert(ret == ESP_OK);                                // Should have had no issues.
}

void spi_driver_display_reset(void)
{
  gpio_set_level(CONFIG_PIN_NUM_RST, 0);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  gpio_set_level(CONFIG_PIN_NUM_RST, 1);
  vTaskDelay(100 / portTICK_PERIOD_MS);
}

//-------------------------------------------------------------------
void spi_driver_display_transaction_finish(spi_display_handle_t spi_display_h, uint32_t transaction_cnt)
{
  spi_transaction_t *rtrans;
  esp_err_t          ret;

  // Wait for all 6 transactions to be done and get back the results.
  for (int x = 0; x < transaction_cnt; x++)
  {
    ret = spi_device_get_trans_result(spi_display_h, &rtrans, portMAX_DELAY);
    assert(ret == ESP_OK);
    // We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
  }
}

void spi_driver_display_write_data(spi_display_handle_t spi_display_h, uint8_t *data, size_t size)
{
  esp_err_t         ret;
  spi_transaction_t transaction;

  while (size > 0)
  {
    uint16_t chunk_size = size > SPI_MAX_DATA_CHUNK_SIZE ? SPI_MAX_DATA_CHUNK_SIZE : size;
    if (chunk_size == 0)
      return;      
                                                                    // no need to send anything
    memset(&transaction, 0, sizeof(transaction));                   // Zero out the transaction

    transaction.length = chunk_size * 8;                            // Len is in bytes, transaction length is in bits.
    transaction.tx_buffer = data;                                   // Data
    transaction.user = (void *)1;                                   // D/C needs to be set to 1

    ret = spi_device_polling_transmit(spi_display_h, &transaction); // Transmit!
    assert(ret == ESP_OK);                                          // Should have had no issues.

    data += chunk_size;
    size -= chunk_size;
  }
}