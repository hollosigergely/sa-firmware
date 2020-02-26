#ifndef DW1000_H_
#define DW1000_H_

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include "math.h"
#include "app_uart.h"
#include "app_error.h"
#include "app_util_platform.h"
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_drv_spi.h"
#include "nrf_soc.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_delay.h"
#include "deca_types.h"
#include "deca_device_api.h"
#include "deca_regs.h"
#include "SEGGER_RTT.h"


#define DW1000_RST		24
#define DW1000_IRQ		19
#define DW1000_SCK_PIN		16
#define DW1000_MISO_PIN		18
#define DW1000_MOSI_PIN		20
#define DW1000_SS_PIN		17

#define UUS_TO_DWT_TIME				63897

uint64_t dwm1000_get_system_time_u64(void);
uint64_t dwm1000_get_rx_timestamp_u64(void);

void dwm1000_timestamp_u64_to_pu8(uint64_t ts, uint8_t* out);

int dwm1000_phy_init();
void dwm1000_phy_release();

#endif /* DW1000_H_ */


