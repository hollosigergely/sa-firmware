#include "accel_service.h"
#include "ble_srv_common.h"
#include "nrf_log.h"
#include <sdk_macros.h>

uint32_t ble_accs_init(ble_accs_t *p_accs)
{
    uint32_t              err_code;
    ble_uuid_t            ble_uuid;
    ble_add_char_params_t add_char_params;

    ble_uuid128_t base_uuid = {ACCS_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_accs->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_accs->uuid_type;
    ble_uuid.uuid = ACCS_UUID_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_accs->service_handle);
    VERIFY_SUCCESS(err_code);

    // Add ranging characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = ACCS_UUID_SENSOR_CHAR;
    add_char_params.uuid_type         = p_accs->uuid_type;
    add_char_params.init_len          = sizeof(df_accel_info_t);
    add_char_params.max_len           = sizeof(df_accel_info_t);
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.notify = 1;

    add_char_params.read_access       = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;

    err_code = characteristic_add(p_accs->service_handle,
                                  &add_char_params,
                                  &p_accs->sensor_char_handles);

    return err_code;
}


void ble_accs_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{

}

uint32_t ble_accs_send_values(uint16_t conn_handle, ble_accs_t * p_accs, df_accel_info_t* acc_data)
{
    if(conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        ble_gatts_hvx_params_t params;
        uint16_t len = sizeof(df_accel_info_t);

        memset(&params, 0, sizeof(params));
        params.type   = BLE_GATT_HVX_NOTIFICATION;
        params.handle = p_accs->sensor_char_handles.value_handle;
        params.p_data = (uint8_t*)acc_data;
        params.p_len  = &len;

        return sd_ble_gatts_hvx(conn_handle, &params);
    }
    else
    {
        return NRF_SUCCESS;
    }
}
