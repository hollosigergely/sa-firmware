#ifndef AGGREGATE_H
#define AGGREGATE_H

#include "utils.h"
#include "dump.h"
#include <map>
#include <string>
#include <vector>
#include <chrono>


class Aggregate : public MessageDump
{
private:
    int     mPacketID;

    std::map<std::string,std::vector<int64_t>>      mData;
    int                                             mData_idx;
    int                                             mLastSFSlot;
    dwm1000_ts_t                                    mLastSFStartTs;
    std::chrono::time_point<std::chrono::system_clock>  mLastProcessInfoTs;

    int  get_sf_slot(uint8_t* buffer);
    void insert_ts(const std::string& col, int sf_id, uint64_t ts, bool this_sf);
    void start_new_sf(int sf_id, dwm1000_ts_t rxts);
public:
    Aggregate();

    void dump(dwm1000_ts_t rx_ts, uint8_t *buffer, size_t length);
    void end();
};

#endif // AGGREGATE_H
