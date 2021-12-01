#include "dump_hex.h"
#include <inttypes.h>
using namespace std;

OstreamHexDump::OstreamHexDump()
{
	this->mStartTS = chrono::system_clock::now();
	this->mPacketID = 0;
}

int OstreamHexDump::count_not_zero(rx_info_t* info, int size)
{
	int count = 0;
	for(int i = 0; i < size; i++)
	{
		if((info[i].rx_ts[0] | info[i].rx_ts[1] | info[i].rx_ts[2] | info[i].rx_ts[3] | info[i].rx_ts[4]) != 0)
			count++;
	}
	return count;
}

void OstreamHexDump::printMessageHex(uint8_t* buffer, size_t length)
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

void OstreamHexDump::dump(dwm1000_ts_t rx_ts, uint8_t *buffer, size_t length)
{
	mPacketID++;

	auto ts = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - mStartTS);
	printf("%05d %010ld %04d\n",
		   mPacketID,
		   ts.count(),
		   (int)length);

	printMessageHex(buffer, length);

	printf("\n\n");
}

