#include "json.h"
#include "timing.h"
#include <sstream>
using namespace std;

#define TAG "aggr"

JSON::JSON()
{
    this->mLastSFSlot = -1;
    this->mStartTS = chrono::system_clock::now();
    this->mPacketID = 0;
}


void JSON::dump(dwm1000_ts_t rx_ts, uint8_t *buffer, size_t length)
{
    mPacketID++;

    auto ts = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - mStartTS);
    double rx_ts_sec = rx_ts.ts / (499.2e6 * 128);

    sf_header_t* hdr = (sf_header_t*)buffer;
	if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE)
	{
		sf_anchor_msg_t* msg = (sf_anchor_msg_t*)buffer;
        dump_anchor_msg(ts, rx_ts_sec, msg);
	}
	else if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_TAG_MESSAGE)
	{
		sf_tag_msg_t* msg = (sf_tag_msg_t*)buffer;
        dump_tag_msg(ts, rx_ts_sec, msg);
	}
}

void JSON::dump_tag_msg(chrono::milliseconds ts, double rx_ts_sec, sf_tag_msg_t* msg) {
    cout << "{";
    cout << "\"ts\":" << ts.count() << ",";
    cout << "\"rx_ts_sec\":" << rx_ts_sec << ",";
    cout << "\"type\": \"tag_message\",";
    cout << "\"group_id\":" << msg->hdr.group_id << ",";
    cout << "\"src_id\":" << msg->hdr.src_id << ",";
    cout << "\"tr_id\":" << (int)msg->tr_id << ",";
    cout << "\"tx_ts\":" << Utils::dwm1000_pu8_to_ts(msg->tx_ts).ts << ",";
    cout << "\"anchor_tss\": [";
    for(int i = 0; i < TIMING_ANCHOR_COUNT; i++)
    {
        if(i > 0)
            cout << ",";
        cout << Utils::dwm1000_pu8_to_ts(msg->anchors[i].rx_ts).ts;
    }
    cout << "]";
    cout << "}";
    cout << endl;
}

void JSON::dump_anchor_msg(chrono::milliseconds ts, double rx_ts_sec, sf_anchor_msg_t* msg) {
    cout << "{";
    cout << "\"ts\":" << ts.count() << ",";
    cout << "\"rx_ts_sec\":" << rx_ts_sec << ",";
    cout << "\"type\": \"anchor_message\",";
    cout << "\"group_id\":" << msg->hdr.group_id << ",";
    cout << "\"src_id\":" << msg->hdr.src_id << ",";
    cout << "\"tr_id\":" << (int)msg->tr_id << ",";
    cout << "\"tx_ts\":" << Utils::dwm1000_pu8_to_ts(msg->tx_ts).ts << ",";
    cout << "\"anchor_tss\": [";
    for(int i = 0; i < TIMING_ANCHOR_COUNT; i++)
    {
        if(i > 0)
            cout << ",";
        cout << Utils::dwm1000_pu8_to_ts(msg->anchors[i].rx_ts).ts;
    }
    cout << "],";
    cout << "\"tag_tss\": [";
    for(int i = 0; i < TIMING_TAG_COUNT; i++)
    {
        if(i > 0)
            cout << ",";
        cout << Utils::dwm1000_pu8_to_ts(msg->tags[i].rx_ts).ts;
    }
    cout << "]";
    cout << "}";
    cout << endl;
}

void JSON::end() {

}