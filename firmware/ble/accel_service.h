#ifndef ACCEL_SERVICE_H
#define ACCEL_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"
#include "timing.h"
#include "data_format.h"

#define BLE_ACCS_DEF(_name)                                                                          \
static ble_accs_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_LBS_BLE_OBSERVER_PRIO,                                                     \
                     ble_accs_on_ble_evt, &_name)

// f45aacdd-00a6-413e-87db-580f0cab9adc
#define ACCS_UUID_BASE        {0xdc, 0x9a, 0xab, 0x0c, 0x0f, 0x58, 0xdb, 0x87, \
                              0x3E, 0x41, 0xa6, 0x00, 0x00, 0x00, 0x5a, 0xf4}
#define ACCS_UUID_SERVICE      0x2000
#define ACCS_UUID_SENSOR_CHAR  0x2002

typedef struct ble_accs_s ble_accs_t;

struct ble_accs_s
{
    uint16_t                    service_handle;
   // ble_gatts_char_handles_t    info_char_handles;
    ble_gatts_char_handles_t    sensor_char_handles;
    uint8_t                     uuid_type;
};

uint32_t ble_accs_init(ble_accs_t * p_rs);
void ble_accs_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
uint32_t ble_accs_send_values(uint16_t conn_handle, ble_accs_t * p_rs, df_accel_info_t* acc_data);


#endif // RANGING_SERVICE_H
