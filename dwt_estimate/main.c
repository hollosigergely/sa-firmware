#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "decadriver/deca_param_types.h"
#include "decadriver/deca_device_api.h"


static dwt_config_t config_long = {
		4,               // Channel number.
		DWT_PRF_64M,     // Pulse repetition frequency.
		DWT_PLEN_1024,    // Preamble length.
		DWT_PAC64,        // Preamble acquisition chunk size. Used in RX only.
		18,               // TX preamble code. Used in TX only.
		18,               // RX preamble code. Used in RX only.
		1,               // Use non-standard SFD (Boolean)
		DWT_BR_850K,      // Data rate.
		DWT_PHRMODE_EXT, // PHY header mode.
		(1024 + 1 + 64 - 64)    // SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only.
};



float dwt_estimate_tx_time( dwt_config_t dwt_config, uint16_t framelength, bool only_rmarker )
{
	int32_t tx_time;
	size_t sym_timing_ind = 0;
	uint16_t shr_len = 0;

	const uint16_t DATA_BLOCK_SIZE  = 330;
	const uint16_t REED_SOLOM_BITS  = 48;

	// Symbol timing LUT
	const size_t SYM_TIM_16MHZ = 0;
	const size_t SYM_TIM_64MHZ = 9;
	const size_t SYM_TIM_110K  = 0;
	const size_t SYM_TIM_850K  = 3;
	const size_t SYM_TIM_6M8   = 6;
	const size_t SYM_TIM_SHR   = 0;
	const size_t SYM_TIM_PHR   = 1;
	const size_t SYM_TIM_DAT   = 2;

	const static uint16_t SYM_TIM_LUT[] = {
		// 16 Mhz PRF
		994, 8206, 8206,  // 0.11 Mbps
		994, 1026, 1026,  // 0.85 Mbps
		994, 1026, 129,   // 6.81 Mbps
		// 64 Mhz PRF
		1018, 8206, 8206, // 0.11 Mbps
		1018, 1026, 1026, // 0.85 Mbps
		1018, 1026, 129   // 6.81 Mbps
	};

	// Find the PHR
	switch( dwt_config.prf ) {
	case DWT_PRF_16M:  sym_timing_ind = SYM_TIM_16MHZ; break;
	case DWT_PRF_64M:  sym_timing_ind = SYM_TIM_64MHZ; break;
	}

	// Find the preamble length
	switch( dwt_config.txPreambLength ) {
	case DWT_PLEN_64:    shr_len = 64;    break;
	case DWT_PLEN_128:  shr_len = 128;  break;
	case DWT_PLEN_256:  shr_len = 256;  break;
	case DWT_PLEN_512:  shr_len = 512;  break;
	case DWT_PLEN_1024: shr_len = 1024;  break;
	case DWT_PLEN_1536: shr_len = 1536;  break;
	case DWT_PLEN_2048: shr_len = 2048;  break;
	case DWT_PLEN_4096: shr_len = 4096;  break;
	}

	// Find the datarate
	switch( dwt_config.dataRate ) {
	case DWT_BR_110K:
		sym_timing_ind  += SYM_TIM_110K;
		shr_len         += 64;  // SFD 64 symbols
		break;
	case DWT_BR_850K:
		sym_timing_ind  += SYM_TIM_850K;
		shr_len         += 8;   // SFD 8 symbols
		break;
	case DWT_BR_6M8:
		sym_timing_ind  += SYM_TIM_6M8;
		shr_len         += 8;   // SFD 8 symbols
		break;
	}

	// Add the SHR time
	tx_time   = shr_len * SYM_TIM_LUT[ sym_timing_ind + SYM_TIM_SHR ];

	// If not only RMARKER, calculate PHR and data
	if( !only_rmarker ) {

		// Add the PHR time (21 bits)
		tx_time  += 21 * SYM_TIM_LUT[ sym_timing_ind + SYM_TIM_PHR ];

		// Bytes to bits
		framelength *= 8;

		// Add Reed-Solomon parity bits
		framelength += REED_SOLOM_BITS * ( framelength + DATA_BLOCK_SIZE - 1 ) / DATA_BLOCK_SIZE;

		// Add the DAT time
		tx_time += framelength * SYM_TIM_LUT[ sym_timing_ind + SYM_TIM_DAT ];

	}

	// Return float seconds
	return (1.0e-9f) * tx_time;

}


int main()
{
	printf("%f\n", dwt_estimate_tx_time(config_long, 10, false));
	return 0;
}
