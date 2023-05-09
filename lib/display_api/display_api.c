#include "display_api.h"
#include "spi_driver_display.h"

#include <string.h>

#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"


// static const uint8_t        DISPLAY_BUF_CNT = 2;
static spi_display_handle_t display_handle  = NULL;

static uint16_t** display_bufs;
static uint16_t* display_buf_write = NULL;
static uint16_t* display_buf_send  = NULL;

static uint16_t* plot_buf = NULL;

static SemaphoreHandle_t sem_lock = NULL;
static SemaphoreHandle_t sem_sign = NULL;

enum plot_size
{
  PLOT_SIZE_X = 50,
  PLOT_SIZE_Y = 4
};

typedef enum display_sizes
{
  DISPLAY_SIZE_X = 240,
  DISPLAY_SIZE_Y = 320,
} display_sizes_t;

static void spi_pre_transfer_callback(spi_transaction_t* spi_trans)
{
  int dc_val = (int)spi_trans->user;
  gpio_set_level(CONFIG_PIN_NUM_DC, dc_val);
}

static inline void swap_buf(uint16_t** left, uint16_t** right)
{
  xSemaphoreTake(sem_lock, portMAX_DELAY);
  uint16_t* left_cp = *left;
  *left = *right;
  *right = *left_cp; 
  xSemaphoreGive(sem_lock);
}

static void displat_send_task(void *params)
{
  while (1)
  {
    xSemaphoreTake(sem_sign, portMAX_DELAY);
    // Функция записи на дисплей
    // ESP_LOGI("DISPLAY API", "start sending");
    
    spi_driver_send_image(display_handle, DISPLAY_SIZE_X, DISPLAY_SIZE_Y, (uint16_t*)display_buf_send);

    vTaskDelay(pdMS_TO_TICKS(20));
    
    // ESP_LOGI("DISPLAY API", "end sending");

  }
}

void display_init()
{
  esp_err_t ret = 0;

  sem_lock = xSemaphoreCreateMutex();
  sem_sign = xSemaphoreCreateBinary();

  spi_bus_config_t spi_cfg = {
    .mosi_io_num = CONFIG_PIN_NUM_MOSI,
    .miso_io_num = -1,
    .sclk_io_num = CONFIG_PIN_NUM_CLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = DISPLAY_SIZE_X * DISPLAY_SIZE_Y*2+8,
    .flags = 0
  };

  spi_device_interface_config_t spi_dev_cfg = {
    .clock_speed_hz = 80*1000*1000,           //Clock out at 10 MHz
    .mode = 0,                                //SPI mode 0
    .spics_io_num = CONFIG_PIN_NUM_CS,        //CS pin
    .queue_size = 7,                          //We want to be able to queue 7 transactions at a time
    .pre_cb = &spi_pre_transfer_callback,     //Specify pre-transfer callback to handle D/C line
  };

  ret = spi_driver_display_init(&spi_cfg, &spi_dev_cfg, &display_handle);
  assert(0 == ret && display_handle != NULL);

  plot_buf = heap_caps_malloc(PLOT_SIZE_X * PLOT_SIZE_Y * sizeof(uint16_t), MALLOC_CAP_DMA);
  memset(plot_buf, 0xFFFF, PLOT_SIZE_X * PLOT_SIZE_Y * sizeof(uint16_t));

  // display_bufs = (uint16_t**)malloc(2 * sizeof(uint16_t*));

  // ESP_LOGI("DISPLAT API", "Totoal dma size %d", heap_caps_get_total_size(MALLOC_CAP_DMA));


  // for (uint32_t i = 0; i < DISPLAY_BUF_CNT; i++) {
  //   display_bufs[i] = heap_caps_malloc( DISPLAY_SIZE_X * DISPLAY_SIZE_Y * sizeof(uint16_t), MALLOC_CAP_DMA);
  //   ESP_LOGI("DISPLAT API", "Alloc buf %lu addr %p", i, display_bufs[i]);
  //   assert(NULL != display_bufs[i]);
    
  //   memset(display_bufs[i], 0, DISPLAY_SIZE_X * DISPLAY_SIZE_Y * sizeof(uint16_t));
  // }

  // ESP_LOGI("DISPLAT API", "Free dma size %d", heap_caps_get_free_size(MALLOC_CAP_DMA));


  // display_buf_write = display_bufs[0];
  // display_buf_send  = display_bufs[1];

  // xTaskCreatePinnedToCore(&displat_send_task, "display send task", 4096, NULL, 4, NULL, 1);
}

void display_draw_pixel(uint32_t x, uint32_t y, uint16_t color)
{
  xSemaphoreTake(sem_lock, portMAX_DELAY);
  display_buf_write[y + x * DISPLAY_SIZE_X] = color;
  xSemaphoreGive(sem_lock);
}

void display_fill_color(uint16_t color)
{
  for (uint32_t i = 0; i < DISPLAY_SIZE_X * DISPLAY_SIZE_Y; i++)
  {
    display_buf_write[i] = color;
  }
}

static void spi_display_send_hor_block(spi_device_handle_t spi, int ypos, uint16_t *data)
{
  static spi_transaction_t trans[6];
  esp_err_t ret;

  int x;
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

  trans[0].tx_data[0] = 0x2A;                    // Column Address Set
  trans[1].tx_data[0] = 0;                       // Start Col High
  trans[1].tx_data[1] = 0;                       // Start Col Low
  trans[1].tx_data[2] = (DISPLAY_SIZE_X) >> 8;   // End Col High
  trans[1].tx_data[3] = (DISPLAY_SIZE_X)&0xff;   // End Col Low
  trans[2].tx_data[0] = 0x2B;                    // Page address set
  trans[3].tx_data[0] = ypos >> 8;               // Start page high
  trans[3].tx_data[1] = ypos & 0xff;             // start page low
  trans[3].tx_data[2] = (ypos + 16) >> 8;        // end page high
  trans[3].tx_data[3] = (ypos + 16) & 0xff;      // end page low
  trans[4].tx_data[0] = 0x2C;                    // memory write
  trans[5].tx_buffer = data;                     // finally send the line data
  trans[5].length = DISPLAY_SIZE_X * 2 * 8 * 16; // Data length, in bits
  trans[5].flags = 0;                            // undo SPI_TRANS_USE_TXDATA flag
  for (x = 0; x < 6; x++)
  {
    ret = spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
    assert(ret == ESP_OK);
  }
}


static void spi_display_block_finish(spi_device_handle_t spi)
{
  spi_transaction_t *rtrans;
  esp_err_t ret;
  // Wait for all 6 transactions to be done and get back the results.
  for (int x = 0; x < 6; x++)
  {
    ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
    assert(ret == ESP_OK);
    // We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
  }
}

void spi_display_fill(uint16_t color)
{
  // Сделать статическое выделение памяти!
  uint16_t *blck = heap_caps_malloc(DISPLAY_SIZE_X * DISPLAY_SIZE_Y * sizeof(uint16_t), MALLOC_CAP_DMA);

  color = color << 8 | color >> 8;

  memset(blck, color, DISPLAY_SIZE_X * DISPLAY_SIZE_Y * sizeof(uint16_t));
  
  spi_driver_send_image(display_handle, DISPLAY_SIZE_X, DISPLAY_SIZE_Y, blck);
  // for (int i = 0; i < 16; i++)
  // {
  //   for (int j = 0; j < DISPLAY_SIZE_X; j++)
  //   {
  //     blck[i * DISPLAY_SIZE_X + j] = color;
  //   }
  // }

  // for (int i = 0; i < DISPLAY_SIZE_X / 16; i++)
  // {
  //   spi_display_send_hor_block(display_handle, i * 16, blck);
  //   spi_display_block_finish(display_handle);
  // }

  heap_caps_free(blck);
}

void display_draw_circle(const pixel_t* center, uint32_t radius, uint16_t color)
{
  int32_t x = 0;
	int32_t y = radius;
	int32_t delta = 1 - 2 * radius;
	int32_t error = 0;

	while(y >= 0)
	{
		display_draw_pixel(center->x + x, center->y + y, color);
		display_draw_pixel(center->x + x, center->y - y, color);
		display_draw_pixel(center->x - x, center->y + y, color);
		display_draw_pixel(center->x - x, center->y - y, color);

		error = 2 * (delta + y) - 1;
		if(delta < 0 && error <= 0)
		{
			++x;
			delta += 2 * x + 1;
			continue;
		}

		error = 2 * (delta - x) - 1;
		if(delta > 0 && error > 0)
		{
			--y;
			delta += 1 - 2 * y;
			continue;
		}

		++x;
		delta += 2 * (x - y);
		--y;
	}
}

void display_send_buffer()
{
  // swap_buf(&display_buf_write, &display_buf_send);
  xSemaphoreTake(sem_lock, portMAX_DELAY);
  uint16_t* left_cp = display_buf_write;
  display_buf_write = display_buf_send;
  display_buf_send = left_cp; 
  xSemaphoreGive(sem_lock);

  xSemaphoreGive(sem_sign);
}

void display_draw_plot(int32_t x, uint16_t color)
{

  if (x < 0 || x > 140)
  {
    return;
  }
  ESP_LOGI("DISPLAY API", "pos x: %ld", x);

  memset(plot_buf, color, PLOT_SIZE_X * PLOT_SIZE_Y * sizeof(uint16_t));

  spi_driver_send_block(display_handle, x, DISPLAY_SIZE_Y - 5, PLOT_SIZE_X, PLOT_SIZE_Y, plot_buf);
}
