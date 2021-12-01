#ifndef DUMP_HEX_H
#define DUMP_HEX_H

#include "dump.h"
#include <chrono>

class OstreamHexDump : public MessageDump
{
private:
    int                                                       mPacketID;
    std::chrono::time_point<std::chrono::system_clock>        mStartTS;

    int             count_not_zero(rx_info_t* info, int size);
    void            printMessageHex(uint8_t* buffer, size_t length);
public:
    OstreamHexDump();

    virtual void dump(dwm1000_ts_t rx_ts, uint8_t* msg, size_t length);
};

#endif // DUMP_STD_H
