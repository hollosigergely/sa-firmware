/**
 * @file nrf_deca.c
 * @brief DWM1000 modul funkciók
 */
#include <math.h>
#include "nrf_drv_gpiote.h"
#include "nrf_deca.h"
#include "log.h"
#include "decadriver/deca_param_types.h"

#define TAG "nrf_deca"

typedef uint64_t uint64;

#define TX_RX_BUF_LENGTH			1024
#define POLL_TX_TO_RESP_RX_DLY_UUS	140

//Frame control:
#define FRAME_CONTROL_RTLS_BEACON	0x85

uint16_t address = 0;


#if (SPI0_ENABLED == 1)
static const nrf_drv_spi_t m_spi_master_0 = NRF_DRV_SPI_INSTANCE(0);
#endif


static inline uint16_t min(uint16_t a, uint16_t b)
{
    return (a<b)?(a):(b);
}


static dwt_config_t config_long = {
        4,               // Channel number.
        DWT_PRF_64M,     // Pulse repetition frequency.
        DWT_PLEN_4096,    // Preamble length.
        DWT_PAC64,        // Preamble acquisition chunk size. Used in RX only.
        18,               // TX preamble code. Used in TX only.
        18,               // RX preamble code. Used in RX only.
        1,               // Use non-standard SFD (Boolean)
        DWT_BR_850K,      // Data rate.
        DWT_PHRMODE_EXT, // PHY header mode.
        (4096 + 1 + 64 - 64)    // SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only.
};

/*
static dwt_config_t config = {
        4,               // Channel number.
        DWT_PRF_64M,     // Pulse repetition frequency.
        DWT_PLEN_128,    // Preamble length.
        DWT_PAC8,        // Preamble acquisition chunk size. Used in RX only.
        18,               // TX preamble code. Used in TX only.
        18,               // RX preamble code. Used in RX only.
        0,               // Use non-standard SFD (Boolean)
        DWT_BR_6M8,      // Data rate.
        DWT_PHRMODE_STD, // PHY header mode.
        (129 + 8 - 8)    // SFD timeout (preamble length + 1 + SFD length - PAC size). Used in RX only.
};
*/

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



static uint8_t m_rx_data[TX_RX_BUF_LENGTH] = {0};

/**
 * @brief DWM 64 bites rendszer óra lekérése
 * @return 64 bites óra
 */
uint64_t dwm1000_get_system_time_u64(void)
{
    uint8_t ts_tab[5];
    uint64_t ts = 0;
    int i;
    dwt_readsystime(ts_tab);
    for (i = 4; i >= 0; i--)
    {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

/**
 * @brief Beérkezési idő lekérése 64 bites felbontásban
 * @return Beérkezési idő
 */
uint64_t dwm1000_get_rx_timestamp_u64(void)
{
    uint8_t ts_tab[5];
    uint64_t ts = 0;
    int i;
    dwt_readrxtimestamp(ts_tab);
    for (i = 4; i >= 0; i--)
    {
        ts <<= 8;
        ts |= ts_tab[i];
    }
    return ts;
}

/**
 * @brief SPI inicializálása
 * @return Inicializálás eredménye
 */
static uint32_t spi_master_init(void)
{
    nrf_drv_spi_config_t config =
    {
            .ss_pin       = NRF_DRV_SPI_PIN_NOT_USED,
            .irq_priority = APP_IRQ_PRIORITY_LOW,
            .orc          = 0xCC,
            .frequency    = NRF_DRV_SPI_FREQ_1M,
            .mode         = NRF_DRV_SPI_MODE_0,
            .bit_order    = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,
    };
    config.sck_pin  = DW1000_SCK_PIN;
    config.mosi_pin = DW1000_MOSI_PIN;
    config.miso_pin = DW1000_MISO_PIN;


    return nrf_drv_spi_init(&m_spi_master_0, &config, NULL, NULL);
}

/**
 * @brief Alvás x milliszekundumig
 * @param time_ms Alvás ideje ms-ben
 */
void deca_sleep(unsigned int time_ms)
{
    nrf_delay_ms(time_ms);
}

/**
 * @brief DWM modul inicializálása
 */
void initDW1000(void)
{
    uint32_t err_code = spi_master_init();
    APP_ERROR_CHECK(err_code);

    nrf_gpio_cfg_output(DW1000_SS_PIN);
    nrf_gpio_pin_set(DW1000_SS_PIN);

    nrf_gpio_cfg_input(DW1000_RST, NRF_GPIO_PIN_NOPULL);

    nrf_gpio_cfg_input(DW1000_IRQ, NRF_GPIO_PIN_PULLDOWN);
}

/**
 * @brief DWM modul újraindítása
 */
void reset_DW1000(void)
{
    nrf_gpio_cfg_output(DW1000_RST);
    nrf_gpio_pin_clear(DW1000_RST);
    nrf_delay_ms(2);
    nrf_gpio_cfg_input(DW1000_RST, NRF_GPIO_PIN_NOPULL);
}

/**
 * @brief SPI beállítása lassú sebességre
 */
void spi_set_rate_low(void)
{
    nrf_drv_spi_uninit(&m_spi_master_0);

    nrf_drv_spi_config_t config =
    {
            .ss_pin       = NRF_DRV_SPI_PIN_NOT_USED,
            .irq_priority = APP_IRQ_PRIORITY_LOW,
            .orc          = 0xCC,
            .frequency    = NRF_DRV_SPI_FREQ_1M,
            .mode         = NRF_DRV_SPI_MODE_0,
            .bit_order    = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,
    };
    config.sck_pin  = DW1000_SCK_PIN;
    config.mosi_pin = DW1000_MOSI_PIN;
    config.miso_pin = DW1000_MISO_PIN;

    nrf_drv_spi_init(&m_spi_master_0, &config, NULL, NULL);
}

/**
 * @brief SPI beállítása gyors sebességre
 */
void spi_set_rate_high(void)
{
    nrf_drv_spi_uninit(&m_spi_master_0);

    nrf_drv_spi_config_t config =
    {
            .ss_pin       = NRF_DRV_SPI_PIN_NOT_USED,
            .irq_priority = APP_IRQ_PRIORITY_LOW,
            .orc          = 0xCC,
            .frequency    = NRF_DRV_SPI_FREQ_8M,
            .mode         = NRF_DRV_SPI_MODE_0,
            .bit_order    = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,
    };
    config.sck_pin  = DW1000_SCK_PIN;
    config.mosi_pin = DW1000_MOSI_PIN;
    config.miso_pin = DW1000_MISO_PIN;

    nrf_drv_spi_init(&m_spi_master_0, &config, NULL, NULL);
}

/**
 * @brief DWM mutex bekapcsolása
 * @return Mutex státusza
 */
decaIrqStatus_t decamutexon(void)
{
    uint8_t temp = 0;
    app_util_critical_region_enter(&temp);

    return temp;
}

/**
 * @brief DWM mutex kikapcsolása
 * @param s Mutex státusza
 */
void decamutexoff(decaIrqStatus_t s)
{
    app_util_critical_region_exit(s);
}


#define MAX_SPI_CHUNK_SIZE  0xFF

/**
 * @brief SPI írás
 * @param headerLength SPI fejléc hossza
 * @param headerBuffer SPI fejléc
 * @param bodylength SPI üzenettörzs hossza
 * @param bodyBuffer SPI üzenettörzs
 * @return Sikeres volt-e az írás
 */
int writetospi(uint16 headerLength, const uint8 *headerBuffer, uint32 bodylength, const uint8 *bodyBuffer)
{
    uint8_t irqs = decamutexon();
    nrf_gpio_pin_clear(DW1000_SS_PIN);

    uint32_t err_code = nrf_drv_spi_transfer(&m_spi_master_0, (uint8 *)headerBuffer, headerLength, m_rx_data, 0);
    if(err_code != NRF_SUCCESS)
    {
		LOGI(TAG,"Write error - header (SPI).\n\r");
    }

    int remain = bodylength;

    while(remain > 0)
    {
/*
        char log_str[80];
        sprintf(log_str, "Index: %ld, %d\n", (bodylength-remain), min(remain, MAX_SPI_CHUNK_SIZE));
        SEGGER_RTT_WriteString(0, log_str);
*/
        err_code = nrf_drv_spi_transfer(&m_spi_master_0, (uint8 *)bodyBuffer+(bodylength-remain), min(remain, MAX_SPI_CHUNK_SIZE), m_rx_data, 0);
        if(err_code != NRF_SUCCESS)
        {
			LOGI(TAG,"Write error - body (SPI).\n\r");
        }

        remain -= min(remain, MAX_SPI_CHUNK_SIZE);
    }

    nrf_gpio_pin_set(DW1000_SS_PIN);
    decamutexoff(irqs);

    return 0;
}

/**
 * @brief SPI olvasás
 * @param headerLength SPI fejléc hossza
 * @param headerBuffer SPI fejléc
 * @param readlength SPI üzenettörzs hossza
 * @param readBuffer SPI üzenettörzs
 * @return Sikeres volt-e az olvasás
 */
int readfromspi(uint16 headerLength,  const uint8 *headerBuffer, uint32 readlength, uint8 *readBuffer)
{
    uint8_t irqs = decamutexon();
    nrf_gpio_pin_clear(DW1000_SS_PIN);

    uint32_t err_code = nrf_drv_spi_transfer(&m_spi_master_0, (uint8 *)headerBuffer, headerLength, m_rx_data, headerLength);

    if(err_code != NRF_SUCCESS)
    {
		LOGI(TAG,"Write error - read header (SPI).\n\r");
    }

    int remain = readlength;

    while(remain > 0)
    {
/*
        char log_str[80];
        sprintf(log_str, "Index: %ld, %d\n", (readlength-remain), min(remain, MAX_SPI_CHUNK_SIZE));
        SEGGER_RTT_WriteString(0, log_str);
*/
        err_code = nrf_drv_spi_transfer(&m_spi_master_0, m_rx_data, min(remain, MAX_SPI_CHUNK_SIZE), readBuffer+(readlength-remain), min(remain, MAX_SPI_CHUNK_SIZE));
        if(err_code != NRF_SUCCESS)
        {
			LOGI(TAG,"Read error (SPI).\n\r");
        }

        remain -= min(remain, MAX_SPI_CHUNK_SIZE);
    }

    nrf_gpio_pin_set(DW1000_SS_PIN);
    decamutexoff(irqs);

    return 0;
}




/**************************************************************************************************************************************************************************************/




response_receive_handler_t initiator_response_receive_handler = NULL;
rtls_beacon_receive_handler_t initiator_rtls_beacon_receive_handler = NULL;

/**
 * @brief Távolságmérés kezdése
 * @param dst_address Célcím
 */
void deca_twr_initiator_send_poll(uint16_t dst_address)
{
    //mac_start_ranging(dst_address);
}

/**
 * @brief Beacon üzenetre figyelés bekapcsolása
 */
void deca_twr_initiator_listen_to_beacon()
{
    dwt_rxenable(0);
}

/**
 * @brief DWM üzenetfogadás időtúllépése
 */
void deca_twr_rxtimeout()
{
    dwt_forcetrxoff();
}

/**
 * @brief DWM modul konfigurációja
 */
void deca_twr_configure()
{
    dwt_configure(&config_long);
}

uint16_t deca_get_address()
{
    return address;
}

int deca_wakeup()
{
    spi_set_rate_low();
    uint8_t tmp[600];
    int result = dwt_spicswakeup(tmp, 600);
    spi_set_rate_high();

    return result;
}

int common_init()
{
    initDW1000();

    deca_wakeup();

    reset_DW1000();

    spi_set_rate_low();
    int init_result = dwt_initialise(DWT_LOADUCODE);
    spi_set_rate_high();

    deca_twr_configure();

	uint32 devid = dwt_readdevid();
	LOGI(TAG,"Decawave magic is %08lX\n",devid);

    if(init_result == DWT_ERROR)
    {
		LOGI(TAG,"Error on dwt_initialise!\n");
        return 1;
    }

	LOGI(TAG,"Configured address: %04X\n", address);

    dwt_setleds(0);

    dwt_setrxantennadelay(0);
    dwt_settxantennadelay(0);

    dwt_setrxaftertxdelay(POLL_TX_TO_RESP_RX_DLY_UUS);
    dwt_setrxtimeout(0);

    //    dwt_write32bitreg(SYS_CFG_ID, SYS_CFG_DIS_DRXB | SYS_CFG_RXWTOE);

    dwt_setsmarttxpower(0);

    dwt_txconfig_t configTx;
    configTx.PGdly = 0x95;
    configTx.power = 0x5F5F5F5F;

    //configTx.PGdly = 0xC5;
    //configTx.power = 0x0E080222;

    //configTx.power = 0x9A9A9A9A;
    //configTx.power = 0x1F1F1F1F;

    dwt_configuretxrf(&configTx);

    float time = dwt_estimate_tx_time(config_long, sizeof(rtls_tdoa_poll_msg_t), false);
	LOGI(TAG,"poll length: %d byte, %f sec\n", sizeof(rtls_tdoa_poll_msg_t), time);

    float time2 = dwt_estimate_tx_time(config_long, sizeof(rtls_tdoa_aggr_msg_t), false);
	LOGI(TAG,"aggr length: %d byte, %f sec\n", sizeof(rtls_tdoa_aggr_msg_t), time2);


    int sys_cfg = dwt_read32bitreg(SYS_CFG_ID);
	LOGI(TAG,"sysconfig: %08X\n", sys_cfg);

    dwt_setinterrupt(DWT_INT_TFRS | DWT_INT_RFCG | DWT_INT_ARFE | DWT_INT_RFSL | DWT_INT_SFDT | DWT_INT_RPHE | DWT_INT_RFCE | DWT_INT_RFTO | DWT_INT_RXPTO | DWT_INT_RXOVRR, 1);

    int sys_mask = dwt_read32bitreg(SYS_MASK_ID);
	LOGI(TAG,"sysmask: %08X\n", sys_mask);

    uint8_t pgdelay = dwt_read8bitoffsetreg(TX_CAL_ID, TC_PGDELAY_OFFSET);
	LOGI(TAG,"pgdelay: %02X\n", pgdelay & 0xFF);

    uint32_t txpower = dwt_read32bitreg(TX_POWER_ID);
	LOGI(TAG,"txpower: %08lX\n", txpower);

    return 0;
}

/**
 * @brief DWM modul inicializálása tag üzemmódban
 * @param handler Távolságmérés callbackje
 * @param beacon_handler Beacon üzenet callbackje
 * @return Sikerült-e az inicializálás
 */
int deca_twr_initiator(uint16_t addr, response_receive_handler_t handler, rtls_beacon_receive_handler_t beacon_handler)
{
    addr = addr & 0x0FFF;
    address = addr;

    initiator_response_receive_handler = handler;
    initiator_rtls_beacon_receive_handler = beacon_handler;

    int result = common_init();
    if(result != 0)
        return result;

    mac_init(address, NULL);
    dwt_setcallbacks(mac_txcallback_impl, mac_rxok_callback_impl, NULL, mac_rxerr_callback_impl);

    return 0;
}




/**************************************************************************************************************************************************************************************/

/**
 * @brief DWM modul inicializálása anchor üzemmódban
 * @return Sikerült-e az inicializálás
 */
int deca_twr_responder(uint16_t addr, mac_tdoa_cb_t tdoa_callback)
{
    addr = addr & 0x00FF;
    addr = addr | 0x8000;
    address = addr;

    int result = common_init();
    if(result != 0)
        return result;

    mac_init(address, tdoa_callback);
    dwt_setcallbacks(mac_txcallback_impl, mac_rxok_callback_impl, NULL, mac_rxerr_callback_impl);

    dwt_configuresleep(DWT_LOADUCODE | DWT_LOADOPSET | DWT_PRESRV_SLEEP | DWT_CONFIG, DWT_WAKE_CS | DWT_SLP_EN);

/*
    uint8_t msg[127] = "The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the lazy dog. The quick brown fox jumps over the l";
    dwt_configcontinuousframemode(4);
    dwt_writetxdata(127, (uint8 *)  msg, 0) ;
    dwt_writetxfctrl(127, 0, 0);
    dwt_starttx(DWT_START_TX_IMMEDIATE);
*/

    return 0;
}




