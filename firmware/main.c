/**
 * @file main.c
 * @brief Főfunkciókat tartalmazó fájl
 */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "app_gpiote.h"
#include "nrf_drv_gpiote.h"
#include "nrf_drv_timer.h"
#include "nrf_drv_saadc.h"
#include "nrf_drv_uart.h"
#include "nrf_delay.h"
#include "app_uart.h"
#include "nrf_drv_twi.h"
#include "SEGGER_RTT.h"
#include "log.h"
#include "utils.h"
#include "boards.h"
#include "app_timer.h"
#include "app_button.h"
#include "app_scheduler.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_sdh.h"
#include "ble_func.h"
#include "uart.h"

#include "address_handler.h"
#include "impl_observer.h"
#include "impl_anchor.h"
#include "impl_tag.h"

#define TAG "main"
//  debugging with gdb: ./JLinkGDBServer -if SWD -device nRF51822
//  ./arm-none-eabi-gdb --tui /home/strauss/munka/ble-decawave/nRF51_SDK_10.0.0_dc26b5e/examples/ble-decawave-tag/ble_app_deca/dev_deca_bt/s110/armgcc/_build/nrf51822_xxac_s110_res.out
//	(gdb) target remote localhost:2331
//	(gdb) load /home/strauss/munka/ble-decawave/nRF51_SDK_10.0.0_dc26b5e/examples/ble-decawave-tag/ble_app_deca/dev_deca_bt/s110/armgcc/_build/nrf51822_xxac_s110_res.out
//	(gdb) monitor reset/go/halt
//	(gdb) monitor memU8 <memory_address>

#define BUTTON_USER                     BSP_BUTTON_0
#define BUTTON_USER_INDEX               0
#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)


static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}


static void leds_init(void)
{
    bsp_board_init(BSP_INIT_LEDS);
}

static void timers_init(void)
{
    // Initialize timer module, making it use the scheduler
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}


static void button_event_handler(uint8_t pin_no, uint8_t button_action)
{
   // ret_code_t err_code;

    switch (pin_no)
    {
        case BUTTON_USER:
            NRF_LOG_INFO("Send button state change");
            /*err_code = ble_lbs_on_button_change(m_conn_handle, &m_lbs, button_action);
            if (err_code != NRF_SUCCESS &&
                err_code != BLE_ERROR_INVALID_CONN_HANDLE &&
                err_code != NRF_ERROR_INVALID_STATE &&
                err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;*/
        break;
    }
}

static void buttons_init(void)
{
    ret_code_t err_code;

    //The array must be static because a pointer to it will be saved in the button handler module.
    static app_button_cfg_t buttons[] =
    {
        {BUTTON_USER, false, BUTTON_PULL, button_event_handler}
    };

    err_code = app_button_init(buttons, ARRAY_SIZE(buttons),
                               BUTTON_DETECTION_DELAY);
    APP_ERROR_CHECK(err_code);

    err_code = app_button_enable();
    APP_ERROR_CHECK(err_code);
}

static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}


int main(void)
{
    log_init();
    utils_init();

    LOGI(TAG,"--> start app <--\n");

	nrf_delay_ms(50);

	uint64_t chipID = addr_handler_get_chip_id();
	LOGI(TAG,"Chip id: %llu\n", chipID);

    char device_name[26];
    addr_handler_get_device_name(device_name);
    LOGI(TAG,"device name: %s\n", device_name);

    APP_SCHED_INIT(APP_TIMER_SCHED_EVENT_DATA_SIZE, 10);

    leds_init();
    timers_init();
    buttons_init();
    power_management_init();

    if(app_button_is_pushed(BUTTON_USER_INDEX)) {
        impl_observer_init();
    }
    else {
        if(!addr_handler_is_anchor())
        {
            ret_code_t err_code;
            err_code = nrf_sdh_enable_request();
            ERROR_CHECK(err_code, NRF_SUCCESS);

            sd_clock_hfclk_request();
            ble_func_init(device_name);

            impl_tag_init();
        }
        else
        {
            impl_anchor_init();
        }
    }


    LOGI(TAG, "exec loop\n");
    for(;;)
    {
        app_sched_execute();
        idle_state_handle();
    }


}


void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
	/*volatile uint32_t r0;
	volatile uint32_t r1;
	volatile uint32_t r2;
	volatile uint32_t r3;
	volatile uint32_t r12;
	volatile uint32_t lr;
	volatile uint32_t pc;
	volatile uint32_t psr;

	r0 = pulFaultStackAddress[ 0 ];
	r1 = pulFaultStackAddress[ 1 ];
	r2 = pulFaultStackAddress[ 2 ];
	r3 = pulFaultStackAddress[ 3 ];

	r12 = pulFaultStackAddress[ 4 ];
	lr = pulFaultStackAddress[ 5 ];
	pc = pulFaultStackAddress[ 6 ];
	psr = pulFaultStackAddress[ 7 ];*/

	for( ;; );
}

void HardFault_Handler( void ) __attribute__( ( naked ) );
void HardFault_Handler(void)
{
	__asm volatile
	(
		" tst lr, #4                                                \n"
		" ite eq                                                    \n"
		" mrseq r0, msp                                             \n"
		" mrsne r0, psp                                             \n"
		" ldr r1, [r0, #24]                                         \n"
		" ldr r2, handler2_address_const                            \n"
		" bx r2                                                     \n"
		" handler2_address_const: .word prvGetRegistersFromStack    \n"
	);
}

