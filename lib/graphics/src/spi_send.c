#include "spi.h"

#include <string.h>

#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"

void spi_reset(void)
{
  gpio_set_level(CONFIG_PIN_NUM_RST, 0);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  gpio_set_level(CONFIG_PIN_NUM_RST, 1);
  vTaskDelay(100 / portTICK_PERIOD_MS);
};

void spi_cmd_send
(
  spi_device_handle_t spi_device_h, 
  uint8_t             cmd
)
{
  assert(spi_device_h);

  esp_err_t ret;
  spi_transaction_t spi_trans;

  memset(&spi_trans, 0, sizeof(spi_trans)); // Zero out the transaction

  spi_trans.length = 8;       // Command is 8 bits
  spi_trans.tx_buffer = &cmd; // The data is the cmd itself
  spi_trans.user = (void *)0; // D/C needs to be set to 0

  ret = spi_device_polling_transmit(spi_device_h, &spi_trans); // Transmit!
  assert(ret == ESP_OK);                              // Should have had no issues.
}

void spi_data_send
(
  spi_device_handle_t spi_device_h,
  const uint8_t*      data,
  int32_t             length
)
{
  assert(spi_device_h);

  esp_err_t ret;
  spi_transaction_t transaction;

  if (length == 0)
    return; // no need to send anything

  memset(&transaction, 0, sizeof(transaction)); // Zero out the transaction

  transaction.length = length * sizeof(*data);                         // Len is in bytes, transaction length is in bits.
  transaction.tx_buffer = data;                         // Data
  transaction.user = (void *)1;                         // D/C needs to be set to 1

  ret = spi_device_polling_transmit(spi_device_h, &transaction); // Transmit!
  assert(ret == ESP_OK);                                // Should have had no issues.
}

void spi_send_image
(
  spi_device_handle_t spi_device_h, 
  uint32_t            size_x, 
  uint32_t            size_y, 
  const uint16_t*     buf
)
{
  esp_err_t ret;
  int x;
  static spi_transaction_t trans[6];
  for (x = 0; x < 6; x++)
  {
    memset(&trans[x], 0, sizeof(spi_transaction_t));
    if ((x & 1) == 0)
    {
      // Even transfers are commands
      trans[x].length = 8;
      trans[x].user = (void *)0;
    }
    else
    {
      // Odd transfers are data
      trans[x].length = 8 * 4;
      trans[x].user = (void *)1;
    }
    trans[x].flags = SPI_TRANS_USE_TXDATA;
  }

  trans[0].tx_data[0] = 0x2A;                              // Column Address Set
  trans[1].tx_data[0] = 0;                                 // Start Col High
  trans[1].tx_data[1] = 0;                                 // Start Col Low
  trans[1].tx_data[2] = (size_x >> 8) & 0xFF;              // End Col High
  trans[1].tx_data[3] = size_x & 0xFF;                     // End Col Low
  trans[2].tx_data[0] = 0x2B;                              // Page address set
  trans[3].tx_data[0] = 0;                                 // Start page high
  trans[3].tx_data[1] = 0;                                 // start page low
  trans[3].tx_data[2] = (size_y >> 8) & 0xFF;              // end page high
  trans[3].tx_data[3] = size_y & 0xFF;                     // end page low
  trans[4].tx_data[0] = 0x2C;                              // memory write
  trans[5].tx_buffer = buf;                            // finally send the line data
  trans[5].length = (size_x) * (size_y) * 2 * 8;   // Data length, in bits
  trans[5].flags = 0;                                      // undo SPI_TRANS_USE_TXDATA flag

  for (x = 0; x < 6; x++)
  {
    ret = spi_device_queue_trans(spi_device_h, &trans[x], portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}

void spi_send_pixels
(
  spi_device_handle_t spi_device_h,
  uint32_t            pos_x,
  uint32_t            pos_y,
  uint32_t            size_x, 
  uint32_t            size_y, 
  const uint8_t*      buf
)
{
  esp_err_t ret;
  int x;
  static spi_transaction_t trans[6];
  for (x = 0; x < 6; x++)
  {
    memset(&trans[x], 0, sizeof(spi_transaction_t));
    if ((x & 1) == 0)
    {
      // Even transfers are commands
      trans[x].length = 8;
      trans[x].user = (void *)0;
    }
    else
    {
      // Odd transfers are data
      trans[x].length = 8 * 4;
      trans[x].user = (void *)1;
    }
    trans[x].flags = SPI_TRANS_USE_TXDATA;
  }

  trans[0].tx_data[0] = 0x2A;                              // Column Address Set
  trans[1].tx_data[0] = (pos_x >> 8) & 0xFF;              // Start Col High
  trans[1].tx_data[1] = pos_x & 0xFF;                                 // Start Col Low
  trans[1].tx_data[2] = ((pos_x + size_x) >> 8) & 0xFF;              // End Col High
  trans[1].tx_data[3] = (pos_x + size_x) & 0xFF;                     // End Col Low
  trans[2].tx_data[0] = 0x2B;                              // Page address set
  trans[3].tx_data[0] = (pos_y >> 8) & 0xFF;                                 // Start page high
  trans[3].tx_data[1] = pos_y & 0xFF;                                 // start page low
  trans[3].tx_data[2] = ((pos_y + size_x) >> 8) & 0xFF;              // end page high
  trans[3].tx_data[3] = (pos_y + size_x) & 0xFF;                     // end page low
  trans[4].tx_data[0] = 0x2C;                              // memory write
  trans[5].tx_buffer = buf;                            // finally send the line data
  trans[5].length = (size_x) * (size_y) * 2 * 8;   // Data length, in bits
  trans[5].flags = 0;                                      // undo SPI_TRANS_USE_TXDATA flag

  for (x = 0; x < 6; x++)
  {
    ret = spi_device_queue_trans(spi_device_h, &trans[x], portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}