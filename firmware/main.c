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


int main(void)
{
	LOGI(TAG,"--> Start app <--\n");

	nrf_delay_ms(50);

	uint64_t chipID = addr_handler_get_chip_id();
	LOGI(TAG,"Chip id: %llu\n", chipID);

	utils_init();

	if(!addr_handler_is_anchor())
	{
		impl_tag_init();
		impl_tag_loop();
	}
	else
	{
		impl_anchor_init();
		impl_anchor_loop();
	}
	//impl_observer_init();
	//impl_observer_loop();
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

