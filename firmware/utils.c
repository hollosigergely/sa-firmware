#include "utils.h"
#include "nrf_gpio.h"
#include "nrf_drv_timer.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "app_util.h"

const static nrf_drv_timer_t m_execution_timer = NRF_DRV_TIMER_INSTANCE(4);

void utils_init()
{
	uint32_t err_code;
	nrf_drv_timer_config_t timer_cfg = {
		.frequency          = NRF_TIMER_FREQ_16MHz,
		.mode               = NRF_TIMER_MODE_TIMER,
		.bit_width          = NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = 0,
		.p_context          = NULL
	};
	err_code = nrf_drv_timer_init(&m_execution_timer, &timer_cfg, NULL);
	APP_ERROR_CHECK(err_code);
}

void utils_start_execution_timer()
{
	nrf_drv_timer_clear(&m_execution_timer);
	nrf_drv_timer_enable(&m_execution_timer);
}

uint32_t utils_stop_execution_timer()
{
	uint32_t counter = nrf_drv_timer_capture(&m_execution_timer, NRF_TIMER_CC_CHANNEL0);
	return counter >> 4;
}

__attribute__((weak))
void my_error_function(int code, const char *filename, const int line)
{
    __disable_irq();

    NRF_LOG_ERROR("FE: %d (%s:%d)", code, filename, line);
    NRF_LOG_FINAL_FLUSH();

    for(;;) {}
}
