#include "main.h"

//------------------------------------------------
static const char *TAG = "main";


//------------------------------------------------
void app_main(void)
{
  joystick_controller_init();
  wifi_init();
  button_pad_init();
  display_init(320, 240);

  start_menu();
}
//------------------------------------------------
