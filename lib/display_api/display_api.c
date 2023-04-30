#include "display_api.h"
#include "spi_driver_display.h"

#include <string.h>

#include "freertos/task.h"
#include "freertos/semphr.h"

typedef uint16_t* display_pixel_buf_t;

static const uint8_t        DISPLAY_BUF_CNT = 2;
static spi_display_handle_t display_handle  = NULL;

static display_pixel_buf_t  display_bufs[DISPLAY_BUF_CNT] = { NULL, NULL };
static display_pixel_buf_t* display_buf_write = NULL;
static display_pixel_buf_t* display_buf_send  = NULL;

static SemaphoreHandle_t sem_lock = NULL;
static SemaphoreHandle_t sem_sign = NULL;

typedef enum display_sizes
{
  DISPLAY_SIZE_X = 320,
  DISPLAY_SIZE_Y = 240,
} display_sizes_t;

static void spi_pre_transfer_callback(spi_transaction_t* spi_trans)
{
  int dc_val = (int)spi_trans->user;
  gpio_set_level(CONFIG_PIN_NUM_DC, dc_val);
}

static void displat_send_task(void *params)
{
  uint32_t buf_idx = 0;

  while (1)
  {
    xSemaphoreTake(sem_sign, portMAX_DELAY);
    // Функция записи на дисплей

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void display_init()
{
  esp_err_t ret = 0;

  spi_bus_config_t spi_cfg = {
    .mosi_io_num = CONFIG_PIN_NUM_MOSI,
    .miso_io_num = -1,
    .sclk_io_num = CONFIG_PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = 16*320*2+8,
    .flags = 0
  };

  spi_device_interface_config_t spi_dev_cfg = {
    .clock_speed_hz = 10*1000*1000,           //Clock out at 10 MHz
    .mode = 0,                                //SPI mode 0
    .spics_io_num = CONFIG_PIN_NUM_CS,        //CS pin
    .queue_size = 7,                          //We want to be able to queue 7 transactions at a time
    .pre_cb = &spi_pre_transfer_callback,     //Specify pre-transfer callback to handle D/C line
  };

  ret = spi_driver_display_init(&spi_cfg, &spi_dev_cfg, &display_handle);
  assert(0 == ret && display_handle != NULL);

  for (uint32_t i = 0; i < DISPLAY_BUF_CNT; i++) {
    display_bufs[i] = (display_pixel_buf_t)heap_caps_malloc(DISPLAY_SIZE_X * DISPLAY_SIZE_Y * sizeof(uint16_t), MALLOC_CAP_DMA);
    assert(NULL != display_bufs[i]);
    
    memset(display_bufs[i], 0, DISPLAY_SIZE_X * DISPLAY_SIZE_Y * sizeof(uint16_t));
  }

  display_buf_write = 
}

void display_draw_pixel(const point_t* pixel_position)
{

}

void display_send_buffer()
{
  xSemaphoreGive(sem_sign);
}