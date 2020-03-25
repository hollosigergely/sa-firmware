#ifndef ACCEL_H_
#define ACCEL_H_

#include <stdint.h>
#include <stdbool.h>
#include "nrf_delay.h"
#include "lis2dh12.h"
#include "app_scheduler.h"
#include "data_format.h"


#define ACCEL_SCHED_EVENT_SIZE            (sizeof(accel_event_t))

void accel_init(app_sched_event_handler_t handler);
void accel_enable();
void accel_disable();

#endif	// ACCEL_H_
