#include "impl_anchor.h"


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
	ANCHOR_STATE__WAITING_SYNC = 0
} anchor_state_t;

const static nrf_drv_timer_t	m_frame_timer = NRF_DRV_TIMER_INSTANCE(1);
static anchor_state_t			m_anchor_state = ANCHOR_STATE__WAITING_SYNC;

static void mac_rxok_callback_impl(const dwt_cb_data_t *data)
{

}

static void mac_rxerr_callback_impl(const dwt_cb_data_t *data)
{
	LOGI(TAG, "rx err\n");

	if(m_anchor_state == ANCHOR_STATE__WAITING_SYNC)
	{
		dwt_rxenable(0);
	}
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

static void frame_timer_event_handler(nrf_timer_event_t event_type, void* p_context)
{
	LOGI(TAG, "FT %d\n", m_anchor_state);

	if(m_anchor_state == ANCHOR_STATE__WAITING_SYNC)
	{
		LOGI(TAG, "sync exp\n");
	}
}


int impl_anchor_init()
{
	LOGI(TAG,"mode: anchor\n");

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;

	gpiote_init();

	LOGI(TAG,"initialize dw1000 phy\n");
	dwm1000_phy_init();

	dwt_setcallbacks(NULL, mac_rxok_callback_impl, NULL, mac_rxerr_callback_impl);

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

	uint32_t time_ticks = nrf_drv_timer_us_to_ticks(&m_frame_timer, TIMING_DISCOVERY_INTERVAL_US);
	nrf_drv_timer_extended_compare(&m_frame_timer, NRF_TIMER_CC_CHANNEL0, time_ticks, NRF_TIMER_SHORT_COMPARE0_STOP_MASK, true);
	nrf_drv_timer_enable(&m_frame_timer);

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


