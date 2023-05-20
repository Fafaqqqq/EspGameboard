#include "main.h"

//------------------------------------------------
static const char *TAG = "main";


//------------------------------------------------
void app_main(void)
{
  joystick_controller_init();
  wifi_init();
  button_pad_init();
  // uint32_t net_cnt;

  // const char** nets = wifi_scan_esp_net(&net_cnt);
  // for (uint32_t i = 0; i < net_cnt; i++)
  // {
  //   ESP_LOGI("main", "SSID: %s", nets[i]);
  // }
  DisplayInit(320, 240);

  StartMenu();
}
//------------------------------------------------
