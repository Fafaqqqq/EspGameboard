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

joystick_status_t joystick_controller_init();
joystick_status_t joystick_data_get(joystick_data_t* joystick_data);

#endif