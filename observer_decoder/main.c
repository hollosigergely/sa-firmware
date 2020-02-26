#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdint.h>
#include <inttypes.h>
#include <byteswap.h>

#include "../firmware/timing.h"

static char buffer[65536];

void myread (int __fd, void *__buf, size_t __nbytes)
{
	do {
		int ret = read(__fd, __buf, __nbytes);
		__nbytes -= ret;
		__buf += ret;
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

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		printf("Wrong argument list!\n");
		exit(1);
	}

	const char* path = argv[1];
	fprintf(stderr, "Using file: %s\n", path);

	int fd = open(path, O_RDONLY);

	uint64_t last_rxts = 0;
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

		printf("%d %"PRIx64" %02.6f %02.6f ",
			   length - 2,
			   rxts,
			   (double)rxts / (499.2e6 * 128),
			   (double)(rxts - last_rxts) / (499.2e6 * 128));
		for(int i = 0; i < length - 2; i++)
		{
			printf("%02X", buffer[i] & 0xFF);
		}

		sf_anchor_msg_t* msg = (sf_anchor_msg_t*)buffer;
		printf("     # AM, src: %04X, trid: %d, txts: %"PRIu64, msg->src_id, msg->tr_id, pu8_to_u64(msg->tx_ts));

		printf("\n");

		last_rxts = rxts;
	}

	return 0;
}
