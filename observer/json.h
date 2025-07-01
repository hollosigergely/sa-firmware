#ifndef JSON_H
#define JSON_H

#include "utils.h"
#include "dump.h"
#include <map>
#include <string>
#include <vector>
#include <chrono>


class JSON : public MessageDump
{
private:
    int                                                         mLastSFSlot;
    std::chrono::time_point<std::chrono::system_clock>          mStartTS;
    int                                                         mPacketID;


    void dump_tag_msg(std::chrono::milliseconds ts, double rx_ts_sec, sf_tag_msg_t* msg);
    void dump_anchor_msg(std::chrono::milliseconds ts, double rx_ts_sec, sf_anchor_msg_t* msg);
public:
    JSON();

    void dump(dwm1000_ts_t rx_ts, uint8_t *buffer, size_t length);
    void end();
};

#endif // JSON_H