#include "ranging_service.h"
#include "ble_srv_common.h"
#include "nrf_log.h"
#include <sdk_macros.h>

uint32_t ble_rs_init(ble_rs_t *p_rs, uint16_t max_ranging_count)
{
    uint32_t              err_code;
    ble_uuid_t            ble_uuid;
    ble_add_char_params_t add_char_params;

    ble_uuid128_t base_uuid = {RS_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_rs->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_rs->uuid_type;
    ble_uuid.uuid = RS_UUID_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_rs->service_handle);
    VERIFY_SUCCESS(err_code);

    // Add example characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = RS_UUID_EXAMPLE_CHAR;
    add_char_params.uuid_type         = p_rs->uuid_type;
    add_char_params.init_len          = sizeof(uint8_t);
    add_char_params.max_len           = sizeof(uint8_t);
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.write  = 1;

    add_char_params.read_access       = SEC_OPEN;
    add_char_params.write_access      = SEC_OPEN;

    err_code = characteristic_add(p_rs->service_handle,
                                  &add_char_params,
                                  &p_rs->example_char_handles);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add ranging characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = RS_UUID_RANGING_CHAR;
    add_char_params.uuid_type         = p_rs->uuid_type;
    add_char_params.init_len          = sizeof(uint16_t);
    add_char_params.max_len           = max_ranging_count * sizeof(uint16_t);
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.notify = 1;

    add_char_params.read_access       = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;

    err_code = characteristic_add(p_rs->service_handle,
                                  &add_char_params,
                                  &p_rs->ranging_char_handles);

    return err_code;
}

static void on_write(ble_rs_t * p_rs, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == p_rs->example_char_handles.value_handle)
    {
        NRF_LOG_INFO("Received data:");
        NRF_LOG_HEXDUMP_INFO(p_evt_write->data, p_evt_write->len);
        //p_lbs->led_write_handler(p_ble_evt->evt.gap_evt.conn_handle, p_lbs, p_evt_write->data[0]);
    }
}


void ble_rs_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_rs_t * p_rs = (ble_rs_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GATTS_EVT_WRITE:
            on_write(p_rs, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}

uint32_t ble_rs_send_ranging(uint16_t conn_handle, ble_rs_t * p_rs, uint16_t* ranging_data, uint16_t ranging_count)
{
    if(conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        ble_gatts_hvx_params_t params;
        uint16_t len = sizeof(uint16_t) * ranging_count;

        memset(&params, 0, sizeof(params));
        params.type   = BLE_GATT_HVX_NOTIFICATION;
        params.handle = p_rs->ranging_char_handles.value_handle;
        params.p_data = (uint8_t*)ranging_data;
        params.p_len  = &len;

        return sd_ble_gatts_hvx(conn_handle, &params);
    }
    else
    {
        return NRF_SUCCESS;
    }
}
