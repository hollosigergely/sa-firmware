#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

#define TIMING_DISCOVERY_INTERVAL_US    2000000

#define TIMING_ANCHOR_COUNT             5
#define TIMING_TAG_COUNT                10

#define TIMING_ANCHOR_MESSAGE_LENGTH_US                       5000
#define TIMING_ANCHOR_MESSAGE_TX_PREFIX_TIME_US               (1051 + 400 + 200)

#define TIMING_TAG_MESSAGE_LENGTH_US           5000

#define TIMING_SUPERFRAME_LENGTH_US     (TIMING_ANCHOR_COUNT * TIMING_ANCHOR_MESSAGE_LENGTH_US + TIMING_TAG_COUNT * TIMING_TAG_MESSAGE_LENGTH_US)
#define TIMING_SUPERFRAME_LENGTH_MS     (TIMING_SUPERFRAME_LENGTH_US/1000)


typedef struct {
    uint8_t         fctrl;
    uint8_t         tr_id;
    uint16_t        src_id;
    uint8_t         tx_ts[5];
} __attribute((__packed__)) sf_anchor_msg_t;

#endif // TIMING_H
