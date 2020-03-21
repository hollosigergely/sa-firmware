#include "uart.h"

#include "nrf_uart.h"
#include "nrf_uarte.h"
#include "nrf_drv_uart.h"
#include "app_uart.h"
#include "nrf_log.h"

static const char* hex_mapping = "0123456789ABCDEF";
void uart_put_as_hex(uint8_t* x, int length)
{
	for(int i = 0; i < length; i++)
	{
		uint8_t byte = x[i];
		app_uart_put(hex_mapping[(byte >> 4) & 0x0F]);
		app_uart_put(hex_mapping[(byte >> 0) & 0x0F]);
	}
}

void uart_put(uint8_t* x, int length)
{
	for(int i = 0; i < length; i++)
	{
        app_uart_put(x[i]);
	}
}

void uart_puts(char* s)
{
	while(*s)
	{
        app_uart_put(*s++);
	}
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

#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 256
void uart_init() {
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
