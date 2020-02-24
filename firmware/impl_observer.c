#include "impl_observer.h"
#include "dwm_utils.h"
#include "log.h"

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpiote.h"

#include "nrf_uart.h"
#include "nrf_uarte.h"
#include "nrf_drv_uart.h"
#include "app_uart.h"
#include "app_error.h"

#define TAG "obs"

#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 256

#define DW1000_RX_BUFFER_SIZE	512
static uint8_t	m_rx_buffer[DW1000_RX_BUFFER_SIZE];


static const char* hex_mapping = "0123456789ABCDEF";
static void uart_put_as_hex(uint8_t* x, int length)
{
	for(int i = 0; i < length; i++)
	{
		uint8_t byte = x[i];
		app_uart_put(hex_mapping[(byte >> 4) & 0x0F]);
		app_uart_put(hex_mapping[(byte >> 0) & 0x0F]);
	}
}

static void uart_put(uint8_t* x, int length)
{
	for(int i = 0; i < length; i++)
	{
		app_uart_put(x[i]);
	}
}

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


static void uart_error_handle(app_uart_evt_t * p_event)
{
	if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
	{
		APP_ERROR_HANDLER(p_event->data.error_communication);
	}
	else if (p_event->evt_type == APP_UART_FIFO_ERROR)
	{
		APP_ERROR_HANDLER(p_event->data.error_code);
	}
}

static void uart_init() {
	const app_uart_comm_params_t comm_params =
	   {
		   11,
		   5,
		   0,
		   0,
		   APP_UART_FLOW_CONTROL_DISABLED,
		   false,
		   NRF_UART_BAUDRATE_1000000  //900000 baudrate
	   };

	uint32_t err_code;
	UNUSED_VARIABLE(err_code);

	APP_UART_FIFO_INIT(&comm_params,
						 UART_RX_BUF_SIZE,
						 UART_TX_BUF_SIZE,
						 uart_error_handle,
						 APP_IRQ_PRIORITY_LOWEST,
						 err_code);
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
