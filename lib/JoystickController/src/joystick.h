#ifndef JOYSTICK_HEADER_H
#define JOYSTICK_HEADER_H

typedef enum {
  joystick_ok,
  joystick_bad_mutex,
  joystick_bad_task,
  joystick_bad_handle,
} joystick_status_t;

typedef enum {
  JOYSTICK_PRESSED,
  JOYSTICK_RELESE,
} joystick_press_t;

typedef struct {
  float x;
  float y;
} joystick_data_t;

typedef void* joystick_handle_t;

joystick_status_t joystick_controller_init(joystick_handle_t* joystick_handle);
joystick_status_t joystick_data_get(joystick_data_t* joystick_data);
void joystick_read_task(void* params);

#endif
// void read_joystick() {
//     adc1_config_width(ADC_WIDTH_BIT_12); // задаем разрядность АЦП
//     adc1_config_channel_atten(JOYSTICK_X_CHANNEL, ADC_ATTEN_DB_11); // задаем усиление для канала оси X
//     adc1_config_channel_atten(JOYSTICK_Y_CHANNEL, ADC_ATTEN_DB_11); // задаем усиление для канала оси Y

//     esp_rom_gpio_pad_select_gpio(JOYSTICK_X_GPIO); // настраиваем GPIO для оси X
//     esp_rom_gpio_pad_select_gpio(JOYSTICK_Y_GPIO); // настраиваем GPIO для оси Y

//     uint32_t joystick_x_center = adc1_get_raw(JOYSTICK_X_CHANNEL); // получаем значение оси X в центре
//     uint32_t joystick_y_center = adc1_get_raw(JOYSTICK_Y_CHANNEL); // получаем значение оси Y в центре

//     while (1) {
//         uint32_t joystick_x_val = adc1_get_raw(JOYSTICK_X_CHANNEL); // считываем значение оси X
//         uint32_t joystick_y_val = adc1_get_raw(JOYSTICK_Y_CHANNEL); // считываем значение оси Y

//         // вычитаем среднее значение для каждой оси, чтобы в центре были значения 0.0
//         *x = roundf(((float)joystick_x_val / 2048.0f - (float)joystick_x_center / 2048.0f) * 100.0f) / 100.0f;
//         *y = roundf(((float)joystick_y_val / 2048.0f - (float)joystick_y_center / 2048.0f) * 100.0f) / 100.0f;


//         ESP_LOGI(TAG, "Joystick X value: %f", *x); // выводим значение оси X
//         ESP_LOGI(TAG, "Joystick Y value: %f", *y); // выводим значение оси Y

//         vTaskDelay(pdMS_TO_TICKS(1000)); // задержка между чтением данных джойстика
//     }
// }