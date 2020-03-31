#include "ranging.h"
#include <string.h>
#include "log.h"
#include "utils.h"

#define TAG "ranging"

#define SPEED_OF_LIGHT					299702547

#define ANT_DLY_CH1                                             16414
#define TX_ANT_DLY                                              ANT_DLY_CH1             //16620 //16436 /* Default antenna delay values for 64 MHz PRF */
#define RX_ANT_DLY                                              ANT_DLY_CH1

#define CORRECT_CLOCK_DIFF(x)   x = ((x) < 0)?((x) + (1ll << 40)):(x)

typedef struct {
    dwm1000_ts_t        anchor_msg_tx_ts;
    dwm1000_ts_t        anchor_msg_rx_ts;
    dwm1000_ts_t        tag_msg_rx_ts;
} anchor_ranging_info_t;

static dwm1000_ts_t                 m_last_tag_tx;
static anchor_ranging_info_t        m_anchor_infos_1[TIMING_ANCHOR_COUNT];
static anchor_ranging_info_t        m_anchor_infos_2[TIMING_ANCHOR_COUNT];
static anchor_ranging_info_t*       m_anchor_infos_new;
static anchor_ranging_info_t*       m_anchor_infos_old;
static uint16_t                     m_anchor_distances[TIMING_ANCHOR_COUNT];
static uint16_t                     m_tag_virtual_addr;

static uint16_t do_ranging(dwm1000_ts_t ts1, dwm1000_ts_t ts2, dwm1000_ts_t ts3, dwm1000_ts_t ts4, dwm1000_ts_t ts5, dwm1000_ts_t ts6)
{
//    LOGI(TAG,"ts1 = %" PRIx64 "\n", ts1);
//    LOGI(TAG,"ts2 = %" PRIx64 "\n", ts2);
//    LOGI(TAG,"ts3 = %" PRIx64 "\n", ts3);
//    LOGI(TAG,"ts4 = %" PRIx64 "\n", ts4);
//    LOGI(TAG,"ts5 = %" PRIx64 "\n", ts5);
//    LOGI(TAG,"ts6 = %" PRIx64 "\n", ts6);

#if TIMING_SUPERFRAME_LENGTH_MS < 60
    uint32_t Tround1 = ts4.ts_low_32 - ts1.ts_low_32;
    uint32_t Tround2 = ts6.ts_low_32 - ts3.ts_low_32;
    uint32_t Treply2 = ts5.ts_low_32 - ts4.ts_low_32;
    uint32_t Treply1 = ts3.ts_low_32 - ts2.ts_low_32;
#else
    int64_t Tround1 = ts4.ts - ts1.ts;
    int64_t Tround2 = ts6.ts - ts3.ts;
    int64_t Treply2 = ts5.ts - ts4.ts;
    int64_t Treply1 = ts3.ts - ts2.ts;

    CORRECT_CLOCK_DIFF(Tround1);
    CORRECT_CLOCK_DIFF(Tround2);
    CORRECT_CLOCK_DIFF(Treply1);
    CORRECT_CLOCK_DIFF(Treply2);
#endif

    int64_t Ra = Tround1 - RX_ANT_DLY - TX_ANT_DLY;
    int64_t Rb = Tround2 - RX_ANT_DLY - TX_ANT_DLY;
    int64_t Da = Treply2 + TX_ANT_DLY + RX_ANT_DLY;
    int64_t Db = Treply1 + TX_ANT_DLY + RX_ANT_DLY;

    float tof_dtu = ((Ra * Rb - Da * Db) / (float)(Ra + Rb + Da + Db));
    float tof = tof_dtu * (float)DWT_TIME_UNITS;
    float distance = tof * SPEED_OF_LIGHT;

    return distance * 1000;
}

void ranging_init(uint16_t tag_virtual_addr)
{
    m_tag_virtual_addr = tag_virtual_addr;
    ERROR_CHECK(tag_virtual_addr <= TIMING_TAG_COUNT, true);

    m_last_tag_tx.ts = 0;
    memset(m_anchor_infos_1, 0, TIMING_ANCHOR_COUNT * sizeof(anchor_ranging_info_t));
    memset(m_anchor_infos_2, 0, TIMING_ANCHOR_COUNT * sizeof(anchor_ranging_info_t));
    m_anchor_infos_new = m_anchor_infos_1;
    m_anchor_infos_old = m_anchor_infos_2;
}

void ranging_on_new_superframe()
{
    m_anchor_infos_new = (m_anchor_infos_new == m_anchor_infos_1)?(m_anchor_infos_2):(m_anchor_infos_1);
    m_anchor_infos_old = (m_anchor_infos_old == m_anchor_infos_1)?(m_anchor_infos_2):(m_anchor_infos_1);
    memset(m_anchor_infos_new, 0, TIMING_ANCHOR_COUNT * sizeof(anchor_ranging_info_t));
    memset(m_anchor_distances, 0, TIMING_ANCHOR_COUNT * sizeof(uint16_t));
}

void ranging_on_tag_tx(dwm1000_ts_t tx_ts)
{
    m_last_tag_tx = tx_ts;
}

void ranging_on_anchor_rx(dwm1000_ts_t rx_ts, sf_anchor_msg_t *msg)
{
    if(msg->hdr.src_id >= TIMING_ANCHOR_COUNT)
    {
        LOGE(TAG,"anchor src id is wrong (%d)\n", msg->hdr.src_id);
        return;
    }

    anchor_ranging_info_t old_ri = m_anchor_infos_old[msg->hdr.src_id];

    anchor_ranging_info_t* new_ri = &m_anchor_infos_new[msg->hdr.src_id];
    new_ri->anchor_msg_rx_ts = rx_ts;
    new_ri->anchor_msg_tx_ts = dwm1000_pu8_to_ts(msg->tx_ts);
    new_ri->tag_msg_rx_ts = dwm1000_pu8_to_ts(msg->tags[m_tag_virtual_addr].rx_ts);

    if(old_ri.anchor_msg_rx_ts.ts != 0)
    {
        uint16_t distance = do_ranging(
                   old_ri.anchor_msg_tx_ts,
                   old_ri.anchor_msg_rx_ts,
                   m_last_tag_tx,
                   new_ri->tag_msg_rx_ts,
                   new_ri->anchor_msg_tx_ts,
                   new_ri->anchor_msg_rx_ts);

        LOGT(TAG, "distance from %d: %" PRIu16 "\n", msg->hdr.src_id, distance);

        m_anchor_distances[msg->hdr.src_id] = distance;
    }
    else
    {
        LOGW(TAG, "ranging failed: %d\n", msg->hdr.src_id);
    }

}


uint16_t *ranging_get_distances()
{
    return m_anchor_distances;
}
