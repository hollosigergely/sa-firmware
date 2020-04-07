#ifndef DUMP_H
#define DUMP_H

#include <stdio.h>
#include "timing.h"
#include "utils.h"

class MessageDump
{
public:
    MessageDump();

    virtual void dump(dwm1000_ts_t rx_ts, uint8_t* msg, size_t length)=0;
    virtual void end() {}
};

#endif // DUMP_H
