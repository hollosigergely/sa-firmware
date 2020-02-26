#include "impl_anchor.h"
#include <stdint.h>

#include "dwm_utils.h"
#include "log.h"
#include "utils.h"

#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpiote.h"

#include "nrf_drv_timer.h"
#include "timing.h"

#define TAG "anchor"

typedef enum {
	ANCHOR_STATE__DISCOVERY = 0,
	ANCHOR_STATE__WAITING_NEXT_SF = 1,
	ANCHOR_STATE__BEFORE_ANCHOR_MSG = 2,
	ANCHOR_STATE__SENDING_ANCHOR_MSG = 3,
	ANCHOR_STATE__AFTER_ANCHOR_MSG = 4,
	ANCHOR_STATE__TAG_FRAME = 5
} anchor_state_t;

const static nrf_drv_timer_t	m_frame_timer = NRF_DRV_TIMER_INSTANCE(1);
static anchor_state_t			m_anchor_state = ANCHOR_STATE__DISCOVERY;
static uint16_t					m_anchor_id = 1;
static uint8_t					m_superframe_id = 0;
static uint8_t					m_rx_buffer[SF_MAX_MESSAGE_SIZE];

static void frame_timer_event_handler(nrf_timer_event_t event_type, void* p_context);
static void trigger_frame_timer(uint32_t us);


static void set_anchor_state(anchor_state_t newstate)
{
	m_anchor_state = newstate;
}

static void mac_rxok_callback_impl(const dwt_cb_data_t *data)
{
	if(m_anchor_state == ANCHOR_STATE__DISCOVERY)
	{
		if(data->datalength - 2 > SF_MAX_MESSAGE_SIZE)
		{
			LOGE(TAG,"oversized msg\n");
			dwt_rxenable(0);
			return;
		}

		dwt_readrxdata(m_rx_buffer, data->datalength - 2, 0);
		uint64_t rxts = dwm1000_get_rx_timestamp_u64();
		uint64_t systime = dwm1000_get_system_time_u64();

		uint32_t slot_time_us = (systime-rxts)/UUS_TO_DWT_TIME + TIMING_ANCHOR_MESSAGE_TX_PREFIX_TIME_US;
		LOGI(TAG,"st %ld\n", slot_time_us);

		sf_header_t* hdr = (sf_header_t*)m_rx_buffer;
		if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE)
		{
			set_anchor_state(ANCHOR_STATE__WAITING_NEXT_SF);

			uint32_t next_sf_start = (TIMING_ANCHOR_COUNT - hdr->src_id) * TIMING_ANCHOR_MESSAGE_LENGTH_US - slot_time_us +
					TIMING_TAG_COUNT * TIMING_TAG_MESSAGE_LENGTH_US;

			LOGI(TAG,"+%ld\n", next_sf_start)

			trigger_frame_timer(next_sf_start);
		}

		dwt_rxenable(0);
	}
	else if(m_anchor_state == ANCHOR_STATE__BEFORE_ANCHOR_MSG ||
			m_anchor_state == ANCHOR_STATE__AFTER_ANCHOR_MSG)
	{

	}

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

static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	if(pin == DW1000_IRQ)
	{
		dwt_isr();
	}
}

static void gpiote_init()
{
	ret_code_t err_code;

	err_code = nrf_drv_gpiote_init();
	APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
	in_config.pull = NRF_GPIO_PIN_PULLDOWN;
	err_code = nrf_drv_gpiote_in_init(DW1000_IRQ, &in_config, gpiote_event_handler);
	APP_ERROR_CHECK(err_code);

	nrf_drv_gpiote_in_event_enable(DW1000_IRQ, true);
}

static void trigger_frame_timer(uint32_t us) {
	nrf_drv_timer_disable(&m_frame_timer);
	nrf_drv_timer_clear(&m_frame_timer);
	uint32_t time_ticks = nrf_drv_timer_us_to_ticks(&m_frame_timer, us);
	nrf_drv_timer_extended_compare(&m_frame_timer, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);
	nrf_drv_timer_enable(&m_frame_timer);
}

static void transmit_anchor_msg() {
	sf_anchor_msg_t	msg;
	msg.hdr.src_id = m_anchor_id;
	msg.hdr.fctrl = SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE;
	msg.tr_id = m_superframe_id;

	uint64_t sys_ts = dwm1000_get_system_time_u64();
	uint32_t tx_ts_32 = (sys_ts + (TIMING_ANCHOR_MESSAGE_TX_PREFIX_TIME_US * UUS_TO_DWT_TIME)) >> 8;
	uint64_t tx_ts = (((uint64_t)(tx_ts_32 & 0xFFFFFFFEUL)) << 8);

	dwm1000_timestamp_u64_to_pu8(tx_ts, msg.tx_ts);

	dwt_forcetrxoff();
	dwt_writetxdata(sizeof(sf_anchor_msg_t) + 2, (uint8_t*)&msg, 0);
	dwt_writetxfctrl(sizeof(sf_anchor_msg_t) + 2, 0, false);
	dwt_setdelayedtrxtime(tx_ts_32);
	if(dwt_starttx(DWT_START_TX_DELAYED) != DWT_SUCCESS)
	{
		LOGE(TAG, "err: starttx\n");
	}
}

static void frame_timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
	LOGT(TAG, "FT %d\n", m_anchor_state);

	if(//m_anchor_state == ANCHOR_STATE__DISCOVERY ||
			m_anchor_state == ANCHOR_STATE__WAITING_NEXT_SF ||
			m_anchor_state == ANCHOR_STATE__TAG_FRAME)
	{
		LOGT(TAG, "SF\n");

		m_superframe_id++;

		uint32_t delay = m_anchor_id * TIMING_ANCHOR_MESSAGE_LENGTH_US;
		if(delay == 0)
		{
			trigger_frame_timer(TIMING_ANCHOR_MESSAGE_LENGTH_US);

			transmit_anchor_msg();

			set_anchor_state(ANCHOR_STATE__SENDING_ANCHOR_MSG);
		}
		else
		{
			trigger_frame_timer(m_anchor_id * TIMING_ANCHOR_MESSAGE_LENGTH_US);
			set_anchor_state(ANCHOR_STATE__BEFORE_ANCHOR_MSG);
		}


	}
	else if(m_anchor_state == ANCHOR_STATE__BEFORE_ANCHOR_MSG)
	{
		trigger_frame_timer(TIMING_ANCHOR_MESSAGE_LENGTH_US);

		transmit_anchor_msg();

		set_anchor_state(ANCHOR_STATE__SENDING_ANCHOR_MSG);
	}
	else if(m_anchor_state == ANCHOR_STATE__SENDING_ANCHOR_MSG)
	{
		trigger_frame_timer((TIMING_ANCHOR_COUNT - m_anchor_id - 1) * TIMING_ANCHOR_MESSAGE_LENGTH_US);
		set_anchor_state(ANCHOR_STATE__AFTER_ANCHOR_MSG);
	}
	else if(m_anchor_state == ANCHOR_STATE__AFTER_ANCHOR_MSG)
	{
		trigger_frame_timer(TIMING_TAG_COUNT * TIMING_TAG_MESSAGE_LENGTH_US);
		set_anchor_state(ANCHOR_STATE__TAG_FRAME);
	}
}


int impl_anchor_init()
{
	LOGI(TAG,"mode: anchor\n");
	LOGI(TAG,"sf length: %d\n", TIMING_SUPERFRAME_LENGTH_MS);

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;

	gpiote_init();

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

	trigger_frame_timer(TIMING_DISCOVERY_INTERVAL_US);

	dwt_rxenable(0);

	return 0;
}

void impl_anchor_loop()
{
	while(1)
	{
		__WFI();
		__WFI();
		/*dwt_forcetrxoff();
		dwt_writetxdata(4, (uint8_t*)"AB", 0);
		dwt_writetxfctrl(4, 0, 1);   // add CRC
		dwt_starttx(DWT_START_TX_IMMEDIATE);

		nrf_delay_ms(500);*/
	}
}


