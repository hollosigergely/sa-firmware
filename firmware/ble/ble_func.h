#ifndef BLE_FUNC_H
#define BLE_FUNC_H

#include <stdint.h>

int ble_func_init(const char *device_name, uint16_t max_ranging_count);
void ble_func_send_ranging(uint16_t* ranging, uint16_t ranging_count);

#endif // BLE_FUNC_H
