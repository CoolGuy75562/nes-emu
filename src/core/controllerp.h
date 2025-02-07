#ifndef CONTROLLERP_H_
#define CONTROLLERP_H_

#include <stdint.h>

uint8_t controller_fetch(void);
void controller_write(uint8_t val);

#endif 
