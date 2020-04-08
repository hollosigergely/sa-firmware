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

void OstreamMessageDump::printMessageHex(uint8_t* buffer, size_t length)
{
	const char* prefix = "    ";
	const int   line_bytes = 16;

	int lines = length/line_bytes + 1;
	for(int line = 0; line < lines; line++)
	{
		printf("%s%06X: ", prefix, line * line_bytes);
		for(int idx = 0; idx < line_bytes; idx++)
		{
			int array_idx = line * line_bytes + idx;
			if(array_idx >= length)
			{
				printf("\n");
				return;
			}

			if(idx % 2 == 0)
				printf(" ");

			printf("%02X", buffer[array_idx]);
		}
		printf("\n");
	}
}

void OstreamMessageDump::dump(dwm1000_ts_t rx_ts, uint8_t *buffer, size_t length)
{
	mPacketID++;

	auto ts = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - mStartTS);
	double rx_ts_sec = rx_ts.ts / (499.2e6 * 128);
	double rx_ts_diff = rx_ts_sec - mLastRXTs;
	rx_ts_diff = (rx_ts_diff < 0)?(rx_ts_diff + 17.207401):(rx_ts_diff);

	printf("%05d %010ld %04d %010" PRIx64 " %9.6f %02.6f\n",
		   mPacketID,
		   ts.count(),
		   (int)length,
		   rx_ts.ts,
		   rx_ts_sec,
		   rx_ts_diff);

	printMessageHex(buffer, length);

	sf_header_t* hdr = (sf_header_t*)buffer;
	if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE)
	{
		sf_anchor_msg_t* msg = (sf_anchor_msg_t*)buffer;
		printf("      # AM, src: %04X, trid: %d, txts: %" PRIu64 ", tags: %d, anchors: %d", msg->hdr.src_id, msg->tr_id, Utils::dwm1000_pu8_to_ts(msg->tx_ts).ts,
			   count_not_zero(msg->tags, TIMING_TAG_COUNT), count_not_zero(msg->anchors, TIMING_ANCHOR_COUNT));
	}
	else if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_TAG_MESSAGE)
	{
		sf_tag_msg_t* msg = (sf_tag_msg_t*)buffer;
		printf("      # TM, src: %04X, trid: %d, txts: %" PRIu64 "", msg->hdr.src_id, msg->tr_id, Utils::dwm1000_pu8_to_ts(msg->tx_ts).ts);
	}

	printf("\n\n");

	mLastRXTs = rx_ts_sec;
}

