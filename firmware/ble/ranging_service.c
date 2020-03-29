#include "ranging_service.h"
#include "ble_srv_common.h"
#include "nrf_log.h"
#include "address_handler.h"
#include <sdk_macros.h>
#include "log.h"
#include "app_scheduler.h"

#define TAG "rs"

static ble_rs_t m_rs;
static df_device_info_t m_device_info;
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;


uint32_t ble_rs_init()
{
    ble_rs_t* p_rs = &m_rs;
    p_rs->mode_callback = NULL;

    m_device_info.group_id = addr_handler_get_group_id();
    m_device_info.dev_id = addr_handler_get_virtual_addr();

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


    // Add info characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = RS_UUID_INFO_CHAR;
    add_char_params.uuid_type         = p_rs->uuid_type;
    add_char_params.init_len          = sizeof(df_device_info_t);
    add_char_params.max_len           = sizeof(df_device_info_t);
    add_char_params.p_init_value      = (uint8_t*)&m_device_info;
    add_char_params.char_props.read   = 1;

    add_char_params.read_access       = SEC_OPEN;

    err_code = characteristic_add(p_rs->service_handle,
                                  &add_char_params,
                                  &p_rs->info_char_handles);

    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    // Add ranging characteristic.
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = RS_UUID_RANGING_CHAR;
    add_char_params.uuid_type         = p_rs->uuid_type;
    add_char_params.init_len          = sizeof(uint16_t);
    add_char_params.max_len           = MAX(sizeof(df_ranging_info_t),sizeof(df_anchor_rx_info_t));
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.notify = 1;

    add_char_params.read_access       = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;

    err_code = characteristic_add(p_rs->service_handle,
                                  &add_char_params,
                                  &p_rs->ranging_char_handles);

    // Add mode characteristic.
    uint8_t mode_value = TAG_MODE_POWERDOWN;
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = RS_UUID_MODE_CHAR;
    add_char_params.uuid_type         = p_rs->uuid_type;
    add_char_params.init_len          = sizeof(uint8_t);
    add_char_params.max_len           = sizeof(uint8_t);
    add_char_params.p_init_value      = &mode_value;
    add_char_params.char_props.read   = 1;
    add_char_params.char_props.write  = 1;

    add_char_params.read_access       = SEC_OPEN;
    add_char_params.write_access      = SEC_OPEN;

    err_code = characteristic_add(p_rs->service_handle,
                                  &add_char_params,
                                  &p_rs->mode_char_handles);

    return err_code;
}

static void on_mode_changed_sched_handler(void *p_event_data, uint16_t event_size)
{
    uint8_t* status = (uint8_t*)p_event_data;

    if(m_rs.mode_callback != NULL &&
            event_size == sizeof(uint8_t))
        m_rs.mode_callback(*status);
}

void ble_rs_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_rs_t * p_rs = (ble_rs_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            LOGI(TAG,"Connected, RS\n");
            m_conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;
        case BLE_GAP_EVT_DISCONNECTED:
            LOGI(TAG,"Disconnected, RS\n");
            m_conn_handle = BLE_CONN_HANDLE_INVALID;

            {
                uint8_t mode = TAG_MODE_POWERDOWN;
                app_sched_event_put((const void*)&mode, sizeof(uint8_t), on_mode_changed_sched_handler);
            }
            break;
        case BLE_GATTS_EVT_WRITE:
            {
                ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
                if(p_evt_write->handle == m_rs.mode_char_handles.value_handle &&
                        p_evt_write->len == sizeof(uint8_t))
                {
                    app_sched_event_put((const void*)&p_evt_write->data[0], sizeof(uint8_t), on_mode_changed_sched_handler);
                }
            }
            break;
        default:
            // No implementation needed.
            break;
    }
}

uint32_t ble_rs_send_ranging(df_ranging_info_t *ranging_data)
{
    if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        ble_gatts_hvx_params_t params;
        uint16_t len = sizeof(df_ranging_info_t);

        memset(&params, 0, sizeof(params));
        params.type   = BLE_GATT_HVX_NOTIFICATION;
        params.handle = m_rs.ranging_char_handles.value_handle;
        params.p_data = (uint8_t*)ranging_data;
        params.p_len  = &len;

        return sd_ble_gatts_hvx(m_conn_handle, &params);
    }
    else
    {
        return NRF_SUCCESS;
    }
}

uint32_t ble_rs_send_anchor_rx_info(df_anchor_rx_info_t *ranging_data)
{
    if(m_conn_handle != BLE_CONN_HANDLE_INVALID)
    {
        ble_gatts_hvx_params_t params;
        uint16_t len = sizeof(df_anchor_rx_info_t);

        memset(&params, 0, sizeof(params));
        params.type   = BLE_GATT_HVX_NOTIFICATION;
        params.handle = m_rs.ranging_char_handles.value_handle;
        params.p_data = (uint8_t*)ranging_data;
        params.p_len  = &len;

        return sd_ble_gatts_hvx(m_conn_handle, &params);
    }
    else
    {
        return NRF_SUCCESS;
    }
}


uint8_t ble_rs_get_uuid_type()
{
    return m_rs.uuid_type;
}

void ble_rs_set_tag_mode_callback(ble_rs_tag_mode_callback_t cb)
{
    m_rs.mode_callback = cb;
}


NRF_SDH_BLE_OBSERVER(m_rs_obs,BLE_LBS_BLE_OBSERVER_PRIO,ble_rs_on_ble_evt, &m_rs);

