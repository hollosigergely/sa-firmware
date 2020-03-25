#ifndef BLE_FUNC_H
#define BLE_FUNC_H

#include <stdint.h>
#include "timing.h"
#include "ranging_service.h"

int ble_func_init(const char *device_name);
void ble_func_send_ranging(df_ranging_info_t* ranging);
void ble_func_send_accel_values(df_accel_info_t* accel_info);

#endif // BLE_FUNC_H
