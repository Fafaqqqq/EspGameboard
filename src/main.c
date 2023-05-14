#include "main.h"

//------------------------------------------------
static const char *TAG = "main";

//------------------------------------------------
void app_main(void)
{
  joystick_controller_init();
  wifi_init();
  DisplayInit(320, 240);
  
  StartMenu();
}
//------------------------------------------------
