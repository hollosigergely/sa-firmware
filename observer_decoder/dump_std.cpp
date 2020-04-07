#include "dump_std.h"
#include <inttypes.h>
using namespace std;

OstreamMessageDump::OstreamMessageDump()
{
	this->mLastRXTs = 0;
	this->mStartTS = chrono::system_clock::now();
	this->mPacketID = 0;
}

int OstreamMessageDump::count_not_zero(rx_info_t* info, int size)
{
	int count = 0;
	for(int i = 0; i < size; i++)
	{
		if((info[i].rx_ts[0] | info[i].rx_ts[1] | info[i].rx_ts[2] | info[i].rx_ts[3] | info[i].rx_ts[4]) != 0)
			count++;
	}
	return count;
}

void OstreamMessageDump::dump(dwm1000_ts_t rx_ts, uint8_t *buffer, size_t length)
{
	mPacketID++;

	auto ts = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - mStartTS);
	double rx_ts_sec = rx_ts.ts / (499.2e6 * 128);
	double rx_ts_diff = rx_ts_sec - mLastRXTs;
	rx_ts_diff = (rx_ts_diff < 0)?(rx_ts_diff + 17.207401):(rx_ts_diff);

	printf("%05d %10ld %4d %010" PRIx64 " %9.6f %02.6f ",
		   mPacketID,
		   ts.count(),
		   (int)length,
		   rx_ts.ts,
		   rx_ts_sec,
		   rx_ts_diff);

	for(int i = 0; i < length - 2; i++)
	{
		printf("%02X", buffer[i] & 0xFF);
	}

	sf_header_t* hdr = (sf_header_t*)buffer;
	if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE)
	{
		sf_anchor_msg_t* msg = (sf_anchor_msg_t*)buffer;
		printf("     # AM, src: %04X, trid: %d, txts: %" PRIu64 ", tags: %d, anchors: %d", msg->hdr.src_id, msg->tr_id, Utils::dwm1000_pu8_to_ts(msg->tx_ts).ts,
			   count_not_zero(msg->tags, TIMING_TAG_COUNT), count_not_zero(msg->anchors, TIMING_ANCHOR_COUNT));
	}
	else if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_TAG_MESSAGE)
	{
		sf_tag_msg_t* msg = (sf_tag_msg_t*)buffer;
		printf("     # TM, src: %04X", msg->hdr.src_id);
	}

	printf("\n");

	mLastRXTs = rx_ts_sec;
}

