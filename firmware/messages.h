#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>

#define __packed __attribute__((packed))

typedef struct {
    uint8_t     msgid:4;
    uint8_t     _unused:4;
} frame_ctrl_t;

typedef struct __packed {
    frame_ctrl_t     fctrl;
    uint8_t          seqid;
    uint16_t         src_id;
    uint8_t          tx_ts[5];
} msg_beacon_t ;

#endif // MESSAGES_H
