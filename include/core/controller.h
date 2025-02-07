#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include <stdint.h>

enum {
  CTRLR_BUTTON_A = 1 << 0,
  CTRLR_BUTTON_B = 1 << 1,
  CTRLR_BUTTON_SELECT = 1 << 2,
  CTRLR_BUTTON_START = 1 << 3,
  CTRLR_BUTTON_UP = 1 << 4,
  CTRLR_BUTTON_DOWN = 1 << 5,
  CTRLR_BUTTON_LEFT = 1 << 6,
  CTRLR_BUTTON_RIGHT = 1 << 7
};

/* registers callback which returns buttons currently being pressed */
void controller_init(uint8_t (*get_pressed_buttons)(void *), void *data);

#endif 
