#ifndef MAIN_MAIN_H_
#define MAIN_MAIN_H_
//---------------------------------------------------------------------
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include <driver/spi_master.h>

#include "esp_err.h"
#include "esp_log.h"

#include "sdkconfig.h"

#include "display_api.h"
#include "joystick.h"
#include "wifi.h"

#include "menu.h"
#include "pong.h"
#include "button_pad.h"

//---------------------------------------------------------------------
#endif /* MAIN_MAIN_H_ */
