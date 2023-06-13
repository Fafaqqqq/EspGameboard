#ifndef BUTTON_PAD_H
#define BUTTON_PAD_H

typedef enum
{
  BUTTON_BLUE, 
  BUTTON_RED,
} button_type_t;

typedef void* button_intr_params;
typedef void  (*button_intr_func)(void*);

void button_pad_init();

int button_register_intr
(
  button_type_t btn_type,
  button_intr_func intr_func,
  button_intr_params intr_params
);

int button_unregister_intr
(
  button_type_t btn_type
);

int  button_pressed
(
  button_type_t btn_type
);

void button_flush
(
  button_type_t btn_type
);

#endif