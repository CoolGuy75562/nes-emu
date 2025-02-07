#include "core/controller.h"
#include "controllerp.h"

#include <stdlib.h>

enum {PARALLEL, SERIAL};

static uint8_t buttons = 0;
static int mode = PARALLEL;

static uint8_t (*get_pressed_buttons)(void *) = NULL;
static void *get_pressed_buttons_data = NULL;

void controller_init(uint8_t (*get_pressed_buttons_cb)(void *), void *data) {
  get_pressed_buttons = get_pressed_buttons_cb;
  get_pressed_buttons_data = data;
}

uint8_t controller_fetch(void) {
  if (mode == PARALLEL) {
    return buttons & 1; // 'A' button
  } else {
    uint8_t val = buttons & 1;
    buttons >>= 1;
    return val;
  }
}

void controller_write(uint8_t val) {
  if (val & 1) {
    mode = PARALLEL;
    buttons = get_pressed_buttons(get_pressed_buttons_data);
  } else {
    mode = SERIAL;
  }
}
