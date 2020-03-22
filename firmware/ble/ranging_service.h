#ifndef RANGING_SERVICE_H
#define RANGING_SERVICE_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#define BLE_RS_DEF(_name)                                                                          \
static ble_rs_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_LBS_BLE_OBSERVER_PRIO,                                                     \
                     ble_rs_on_ble_evt, &_name)

// f45aacdd-00a6-413e-87db-580f0cab9adc
#define RS_UUID_BASE        {0xdc, 0x9a, 0xab, 0x0c, 0x0f, 0x58, 0xdb, 0x87, \
                              0x3E, 0x41, 0xa6, 0x00, 0x00, 0x00, 0x5a, 0xf4}
#define RS_UUID_SERVICE      0x1524
#define RS_UUID_EXAMPLE_CHAR 0x1525
#define RS_UUID_RANGING_CHAR 0x1526

typedef struct ble_rs_s ble_rs_t;

struct ble_rs_s
{
    uint16_t                    service_handle;      /**< Handle of Ranging Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t    example_char_handles;    /**< Handles related to the Example Characteristic. */
    ble_gatts_char_handles_t    ranging_char_handles;
    uint8_t                     uuid_type;           /**< UUID type for the Ranging Service. */
};

uint32_t ble_rs_init(ble_rs_t * p_rs, uint16_t max_ranging_count);
void ble_rs_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);
uint32_t ble_rs_send_ranging(uint16_t conn_handle, ble_rs_t * p_rs, uint16_t* ranging_data, uint16_t ranging_count);


#endif // RANGING_SERVICE_H
