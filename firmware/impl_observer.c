#include "impl_observer.h"
#include "dwm_utils.h"
#include "log.h"

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpiote.h"

#include "uart.h"
#include "app_error.h"

#define TAG "obs"

#define DW1000_RX_BUFFER_SIZE	512
static uint8_t	m_rx_buffer[DW1000_RX_BUFFER_SIZE];

static void mac_rxok_callback_impl(const dwt_cb_data_t *data)
{
	if(data->datalength - 2 > DW1000_RX_BUFFER_SIZE)
	{
		dwt_rxenable(0);
		return;
	}

	dwt_readrxdata(m_rx_buffer, data->datalength - 2, 0);
	uint64_t rxts = dwm1000_get_rx_timestamp_u64();

	uart_put((uint8_t*)"x",1);
	uart_put((uint8_t*)&data->datalength, sizeof(uint16_t));
	uart_put((uint8_t*)&rxts, sizeof(uint64_t));
	uart_put((uint8_t*)m_rx_buffer, data->datalength - 2);

	dwt_rxenable(0);
}

static void mac_rxerr_callback_impl(const dwt_cb_data_t *data)
{
	uart_put((uint8_t*)"x",1);
	app_uart_put(0);
	app_uart_put(0);

	dwt_rxenable(0);
}

static void gpiote_event_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	if(pin == DW1000_IRQ)
	{
		LOGT(TAG,"DW1000 IRQ\n");

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



void impl_observer_init()
{
	LOGI(TAG,"mode: obs\n");

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;

	gpiote_init();
	uart_init();

	LOGI(TAG,"initialize dw1000 phy\n");
	dwm1000_phy_init();

	dwt_setcallbacks(NULL, mac_rxok_callback_impl, NULL, mac_rxerr_callback_impl);
	dwt_rxenable(0);
}

void impl_observer_loop()
{
	while(1) {}
}
