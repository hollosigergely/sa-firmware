#ifndef ACCEL_H_
#define ACCEL_H_

#include <stdint.h>
#include <stdbool.h>
#include "nrf_delay.h"

int16_t accel_get_accel_data(int idx);
void accel_refresh_data();

void accel_init(void);
void accel_enable();
void accel_disable();

#endif	// ACCEL_H_
