#include "impl_tag.h"
#include <stdint.h>

#include "app_scheduler.h"
#include "dwm_utils.h"
#include "log.h"
#include "utils.h"
#include "address_handler.h"

#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpiote.h"

#include "nrf_drv_timer.h"
#include "timing.h"
#include "uart.h"

#include "ranging.h"
#include "ble_func.h"

#define TAG "tag"



#define SYNC_COMPENSATION_CONST_US	150

typedef enum {
	TAG_STATE__DISCOVERY = 0,
	TAG_STATE__SYNCHRONIZED = 1
} tag_state_t;

typedef enum
{
	EVENT_RX = 1,
	EVENT_TIMER = 2,
	EVENT_SF_BEGIN = 3
} event_type_t;

const static nrf_drv_timer_t	m_frame_timer = NRF_DRV_TIMER_INSTANCE(1);
static int32_t					m_frame_timer_compensation_us = 0;
static tag_state_t				m_tag_state = TAG_STATE__DISCOVERY;
static uint16_t					m_tag_id = 0;
static uint8_t					m_superframe_id = 0;
static uint8_t					m_rx_buffer[SF_MAX_MESSAGE_SIZE];
static uint8_t					m_received_sync_messages_count = 0;
static uint8_t					m_unsynced_sf_count = 0;

static void frame_timer_event_handler(nrf_timer_event_t event_type, void* p_context);
static void restart_frame_timer();
static void set_frame_timer(uint32_t us);
static void event_handler(event_type_t event_type, const uint8_t* data, uint16_t datalength);


static void set_tag_state(tag_state_t newstate)
{
	m_tag_state = newstate;
}

inline static uint32_t get_slot_time_by_message(dwm1000_ts_t rx_ts)
{
    uint32_t rx_ts_32 = rx_ts.ts >> 8;
    uint32_t systime_32 = dwm1000_get_system_time_u64().ts >> 8;

	return (systime_32-rx_ts_32)/(UUS_TO_DWT_TIME/256) + TIMING_MESSAGE_TX_PREFIX_TIME_US;
}

static void mac_rxok_callback_impl(const dwt_cb_data_t *data)
{
	if(data->datalength - 2 > SF_MAX_MESSAGE_SIZE)
	{
		LOGE(TAG,"oversized msg\n");
		dwt_rxenable(0);
		return;
	}

	dwt_readrxdata(m_rx_buffer, data->datalength - 2, 0);

	event_handler(EVENT_RX, m_rx_buffer, data->datalength - 2);

	dwt_rxenable(0);
}

static void mac_rxerr_callback_impl(const dwt_cb_data_t *data)
{
	LOGI(TAG, "rx err\n");

	dwt_rxenable(0);
}

static void mac_txok_callback_impl(const dwt_cb_data_t* data)
{
	dwt_rxenable(0);
}

static void restart_frame_timer() {
	m_frame_timer_compensation_us = 0;

	nrf_drv_timer_pause(&m_frame_timer);
	nrf_drv_timer_clear(&m_frame_timer);
	uint32_t time_ticks = nrf_drv_timer_us_to_ticks(&m_frame_timer, TIMING_SUPERFRAME_LENGTH_US);
	nrf_drv_timer_compare(&m_frame_timer, NRF_TIMER_CC_CHANNEL1, time_ticks, true);
	nrf_drv_timer_resume(&m_frame_timer);
}

static void set_frame_timer(uint32_t us) {
	nrf_drv_timer_pause(&m_frame_timer);
	uint32_t time_ticks = nrf_drv_timer_us_to_ticks(&m_frame_timer, us - m_frame_timer_compensation_us);
	nrf_drv_timer_compare(&m_frame_timer, NRF_TIMER_CC_CHANNEL0, time_ticks, true);
	nrf_drv_timer_resume(&m_frame_timer);
}

static void compensate_frame_timer(uint32_t c)
{
	uint32_t state = nrf_drv_timer_capture(&m_frame_timer, NRF_TIMER_CC_CHANNEL2) >> 4;
	LOGT(TAG,"state: %ld\n", state);

	m_frame_timer_compensation_us = c - state;

	nrf_drv_timer_pause(&m_frame_timer);
	uint32_t time_ticks = nrf_drv_timer_us_to_ticks(&m_frame_timer, TIMING_SUPERFRAME_LENGTH_US - m_frame_timer_compensation_us);
	nrf_drv_timer_compare(&m_frame_timer, NRF_TIMER_CC_CHANNEL1, time_ticks, true);
	nrf_drv_timer_resume(&m_frame_timer);
}

static void log_anchor_message(sf_anchor_msg_t* msg, uint64_t rx_ts)
{
	char s[40];
	sprintf(s, "1 %d %d %02x%02x%02x%02x%02x\n", msg->hdr.src_id, msg->tr_id, msg->tx_ts[4], msg->tx_ts[3], msg->tx_ts[2], msg->tx_ts[1], msg->tx_ts[0]);
	uart_puts(s);
	sprintf(s, "2 %d %d %"PRIx64"\n", msg->hdr.src_id, msg->tr_id, rx_ts);
	uart_puts(s);

	for(int i = 0; i < TIMING_TAG_COUNT; i++)
	{
		if((msg->tags[i].rx_ts[4] | msg->tags[i].rx_ts[3] | msg->tags[i].rx_ts[2] | msg->tags[i].rx_ts[1] | msg->tags[i].rx_ts[0]) != 0)
		{
			sprintf(s, "4 %d %d %d %02x%02x%02x%02x%02x\n", msg->hdr.src_id,i, msg->tr_id, msg->tags[i].rx_ts[4], msg->tags[i].rx_ts[3], msg->tags[i].rx_ts[2], msg->tags[i].rx_ts[1], msg->tags[i].rx_ts[0]);
			uart_puts(s);
		}
	}

	for(int i = 0; i < TIMING_ANCHOR_COUNT; i++)
	{
		if((msg->anchors[i].rx_ts[4] | msg->anchors[i].rx_ts[3] | msg->anchors[i].rx_ts[2] | msg->anchors[i].rx_ts[1] | msg->anchors[i].rx_ts[0]) != 0)
		{
			sprintf(s, "5 %d %d %d %02x%02x%02x%02x%02x\n", msg->hdr.src_id, i, msg->tr_id, msg->anchors[i].rx_ts[4], msg->anchors[i].rx_ts[3], msg->anchors[i].rx_ts[2], msg->anchors[i].rx_ts[1], msg->anchors[i].rx_ts[0]);
			uart_puts(s);
		}
	}
}

static void log_tag_message(sf_tag_msg_t* msg, uint64_t tx_ts)
{
	char s[40];
	sprintf(s, "3 %d %"PRIx64"\n", m_superframe_id, tx_ts);

	uart_puts(s);
}

static void transmit_tag_msg() {
	sf_tag_msg_t	msg;
	msg.hdr.src_id = m_tag_id;
	msg.hdr.fctrl = SF_HEADER_FCTRL_MSG_TYPE_TAG_MESSAGE;

    uint64_t sys_ts = dwm1000_get_system_time_u64().ts;
	uint32_t tx_ts_32 = (sys_ts + (TIMING_MESSAGE_TX_PREFIX_TIME_US * UUS_TO_DWT_TIME)) >> 8;
    dwm1000_ts_t tx_ts = { .ts = (((uint64_t)(tx_ts_32 & 0xFFFFFFFEUL)) << 8) };

	//dwm1000_timestamp_u64_to_pu8(tx_ts, msg.tx_ts);

	dwt_forcetrxoff();
	dwt_writetxdata(sizeof(sf_tag_msg_t) + 2, (uint8_t*)&msg, 0);
	dwt_writetxfctrl(sizeof(sf_tag_msg_t) + 2, 0, false);
	dwt_setdelayedtrxtime(tx_ts_32);
	if(dwt_starttx(DWT_START_TX_DELAYED) != DWT_SUCCESS)
	{
		LOGE(TAG, "err: starttx\n");
	}

    log_tag_message(&msg, tx_ts.ts);

    ranging_on_tag_tx(tx_ts);
}

static void ranging_scheduler_event_handler(void *p_event_data, uint16_t event_size)
{
    LOGI(TAG, "sending ranging (%d)\n", event_size);
    ble_func_send_ranging(p_event_data, event_size);
}


static void event_handler(event_type_t event_type, const uint8_t* data, uint16_t datalength)
{
	LOGT(TAG, "S %d, E %d (%ld)\n", m_tag_state, event_type, (nrf_drv_timer_capture(&m_frame_timer, NRF_TIMER_CC_CHANNEL2) >> 4));

	if(event_type == EVENT_SF_BEGIN)
	{
		restart_frame_timer();
		LOGT(TAG, "SF\n");

		m_received_sync_messages_count = 0;
        ranging_on_new_superframe();
	}

	if(m_tag_state == TAG_STATE__DISCOVERY)
	{
		if(event_type == EVENT_RX)
		{
			// Found anchor message, sync

			sf_header_t* hdr = (sf_header_t*)data;
			if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE)
			{
				set_tag_state(TAG_STATE__SYNCHRONIZED);

				uint32_t slot_time_us = get_slot_time_by_message(dwm1000_get_rx_timestamp_u64());
				uint32_t sf_time_us = hdr->src_id * TIMING_ANCHOR_MESSAGE_LENGTH_US + slot_time_us;

				compensate_frame_timer(sf_time_us);

				LOGT(TAG,"SFT %ld\n", sf_time_us)

				sf_anchor_msg_t* msg = (sf_anchor_msg_t*)data;
				m_superframe_id = msg->tr_id;
			}
		}
	}
	else if(m_tag_state == TAG_STATE__SYNCHRONIZED)
	{
		if(event_type == EVENT_RX)
		{
			sf_header_t* hdr = (sf_header_t*)data;
			if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE)
			{
                dwm1000_ts_t rx_ts = dwm1000_get_rx_timestamp_u64();
				uint32_t slot_time_us = get_slot_time_by_message(rx_ts);
				uint32_t sf_time_us = hdr->src_id * TIMING_ANCHOR_MESSAGE_LENGTH_US + slot_time_us;

				compensate_frame_timer(sf_time_us);

				LOGT(TAG,"SFT %ld\n", sf_time_us)

				sf_anchor_msg_t* msg = (sf_anchor_msg_t*)data;
				m_superframe_id = msg->tr_id;

				set_frame_timer(TIMING_ANCHOR_COUNT * TIMING_ANCHOR_MESSAGE_LENGTH_US + m_tag_id * TIMING_TAG_MESSAGE_LENGTH_US);

				m_received_sync_messages_count++;
				m_unsynced_sf_count = 0;
                log_anchor_message(msg,rx_ts.ts);
                ranging_on_anchor_rx(rx_ts, msg);
			}
		}
		else if(event_type == EVENT_TIMER)
		{
			if(m_received_sync_messages_count == 0)
			{
				m_unsynced_sf_count++;
                LOGW(TAG, "sync warn\n");

				if(m_unsynced_sf_count > 5)
				{
					set_tag_state(TAG_STATE__DISCOVERY);
                    LOGE(TAG, "sync lost\n");

					return;
				}
			}


			transmit_tag_msg();

            app_sched_event_put(ranging_get_distances(), TIMING_ANCHOR_COUNT, ranging_scheduler_event_handler);
		}
	}
}

static void frame_timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
	if(event_type == NRF_TIMER_EVENT_COMPARE0)
		event_handler(EVENT_TIMER, NULL, 0);
	else if(event_type == NRF_TIMER_EVENT_COMPARE1)
		event_handler(EVENT_SF_BEGIN, NULL, 0);
}






int impl_tag_init()
{
	m_tag_id = addr_handler_get_virtual_addr();
	if(m_tag_id == 0xFFFF)
	{
		LOGE(TAG, "no address specified\n");
		for(;;);
	}

	if(m_tag_id >= TIMING_TAG_COUNT)
	{
		LOGE(TAG, "tag count reached\n");
		for(;;);
	}

	LOGI(TAG,"mode: tag\n");
	LOGI(TAG,"addr: %04X\n", m_tag_id);
	LOGI(TAG,"sf length: %d\n", TIMING_SUPERFRAME_LENGTH_MS);

    //NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    //NRF_CLOCK->TASKS_HFCLKSTART = 1;

    ranging_init(m_tag_id);

    dwm1000_irq_enable();
    uart_init();

	LOGI(TAG,"initialize dw1000 phy\n");
	dwm1000_phy_init();

	dwt_setcallbacks(mac_txok_callback_impl, mac_rxok_callback_impl, NULL, mac_rxerr_callback_impl);

	uint32_t err_code;
	nrf_drv_timer_config_t timer_cfg = {
		.frequency          = NRF_TIMER_FREQ_16MHz,
		.mode               = NRF_TIMER_MODE_TIMER,
		.bit_width          = NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = 2,
		.p_context          = NULL
	};
	err_code = nrf_drv_timer_init(&m_frame_timer, &timer_cfg, frame_timer_event_handler);
	APP_ERROR_CHECK(err_code);
	nrf_drv_timer_enable(&m_frame_timer);
	nrf_drv_timer_pause(&m_frame_timer);

	restart_frame_timer();

	dwt_rxenable(0);

	return 0;
}



