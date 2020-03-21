#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdint.h>
#include <inttypes.h>
#include <byteswap.h>
#include <chrono>
using namespace std;

#include "../firmware/timing.h"

static char buffer[65536];

void myread (int __fd, void *__buf, size_t __nbytes)
{
	uint8_t* buf = (uint8_t*)__buf;
	do {
		int ret = read(__fd, buf, __nbytes);
		__nbytes -= ret;
		buf += ret;
	} while(__nbytes != 0);
}

uint64_t pu8_to_u64(uint8_t* ts_tab)
{
	uint64_t ts = 0;
	int i;
	for (i = 4; i >= 0; i--)
	{
		ts <<= 8;
		ts |= ts_tab[i];
	}
	return ts;
}

int count_not_zero(rx_info_t* info, int size)
{
	int count = 0;
	for(int i = 0; i < size; i++)
	{
		if((info[i].rx_ts[0] | info[i].rx_ts[1] | info[i].rx_ts[2] | info[i].rx_ts[3] | info[i].rx_ts[4]) != 0)
			count++;
	}
	return count;
}

int main(int argc, char** argv)
{
    int fd = STDIN_FILENO;

	auto start_ts = chrono::system_clock::now();

	uint32_t last_rxts = 0;
	for(;;) {
		do {
			read(fd, buffer, 1);
		} while(buffer[0] != 'x');

		uint16_t length;
		uint64_t rxts;

		myread(fd, &length, sizeof(uint16_t));
		if(length == 0)
		{
			printf("0\n");
			continue;
		}

		myread(fd, &rxts, sizeof(uint64_t));
		myread(fd, buffer, length - 2);

		uint32_t rxts_32 = rxts >> 8;

		auto ts = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start_ts);
		printf("%10ld %4d %10" PRIx64 " %9.6f %02.6f ",
			   ts.count(),
			   length - 2,
			   rxts,
			   (double)rxts / (499.2e6 * 128),
			   (double)(rxts_32 - last_rxts) / (499.2e6 / 2));
		for(int i = 0; i < length - 2; i++)
		{
			printf("%02X", buffer[i] & 0xFF);
		}

		sf_header_t* hdr = (sf_header_t*)buffer;

		if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE)
		{
			sf_anchor_msg_t* msg = (sf_anchor_msg_t*)buffer;
			printf("     # AM, src: %04X, trid: %d, txts: %" PRIu64 ", tags: %d, anchors: %d", msg->hdr.src_id, msg->tr_id, pu8_to_u64(msg->tx_ts),
				   count_not_zero(msg->tags, TIMING_TAG_COUNT), count_not_zero(msg->anchors, TIMING_ANCHOR_COUNT));
		}
		else if(hdr->fctrl == SF_HEADER_FCTRL_MSG_TYPE_TAG_MESSAGE)
		{
			sf_tag_msg_t* msg = (sf_tag_msg_t*)buffer;
			printf("     # TM, src: %04X", msg->hdr.src_id);
		}

		printf("\n");

		last_rxts = rxts >> 8;
	}

	return 0;
}
