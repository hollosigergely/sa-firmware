#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdint.h>
#include <inttypes.h>
#include <byteswap.h>
#include <chrono>
#include <signal.h>
#include <memory>
#include "utils.h"
#include "dump_std.h"
#include "aggregate.h"
#include "crc.h"
using namespace std;

#include "timing.h"
#define TAG "main"
#define CHECK_LENGTH(x,size) do { if(x != size) WARN("length warn (" << x << " != " << size << ")"); } while(0);

static char buffer[65536];
static bool isrunning = true;

int myread (int __fd, void *__buf, size_t __nbytes)
{
	if(!isrunning)
	{
		for(int i = 0; i < __nbytes; i++)
			((uint8_t*)__buf)[i] = 'x';
		return 0;
	}

	uint8_t* buf = (uint8_t*)__buf;
	do {
		int ret = read(__fd, buf, __nbytes);
		if(ret < 0)
			return ret;

		if(ret == 0)
		{
			isrunning = false;
		}

		__nbytes -= ret;
		buf += ret;
	} while(__nbytes != 0 && isrunning);

	return 0;
}

void sigint_handler(int s) {
	isrunning = false;
}

void print_usage(char* argv0) {
	fprintf(stderr, "Usage: %s [-m <dump|aggr>]\n", argv0);
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
	string	mode = "dump";
	int		opt;
	while ((opt = getopt(argc, argv, "m:")) != -1) {
	   switch (opt) {
	   case 'm':
			mode = optarg;

			if(mode != "dump" && mode != "aggr")
			{
				print_usage(argv[0]);
			}

			break;
	   default: /* '?' */
		   print_usage(argv[0]);
	   }
	}

	//int fd = open("/tmp/build-observer_decoder-GCC_Desktop-Default/a",O_RDONLY);//STDIN_FILENO;
	int fd = STDIN_FILENO;

	std::shared_ptr<MessageDump> processor;
	if(mode == "aggr") {
		processor = make_shared<Aggregate>();
	}
	else {
		processor = make_shared<OstreamMessageDump>();
	}

	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = sigint_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	for(;isrunning;) {
		do {
			myread(fd, buffer, 1);
		} while(buffer[0] != 'x' && isrunning);

		uint16_t length_u16;

		myread(fd, &length_u16, sizeof(uint16_t));
		if(length_u16 <= 0)
		{
			WARN(TAG,"length <= 0 (" << length_u16 << ")");
			continue;
		}

		dwm1000_ts_t rxts;
		uint8_t crc_rx;
		myread(fd, &rxts, sizeof(uint64_t));
		myread(fd, buffer, length_u16);
		myread(fd, &crc_rx, sizeof(uint8_t));

		crc_t crc = crc_init();
		crc = crc_update(crc, &length_u16, sizeof(uint16_t));
		crc = crc_update(crc, &rxts.ts, sizeof(uint64_t));
		crc = crc_update(crc, buffer, length_u16);
		crc = crc_finalize(crc);

		if(crc != crc_rx)
		{
			WARN(TAG,"crc error");
			continue;
		}

		sf_header_t* hdr = (sf_header_t*)buffer;
		switch(hdr->fctrl)
		{
		case SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE:
			if(length_u16 != sizeof(sf_anchor_msg_t))
			{
				WARN(TAG,"anchor message length warn (" << length_u16 << " != " << sizeof(sf_anchor_msg_t) << ")");
				continue;
			}
			break;
		case SF_HEADER_FCTRL_MSG_TYPE_TAG_MESSAGE:
			if(length_u16 != sizeof(sf_tag_msg_t))
			{
				WARN(TAG,"tag message length warn (" << length_u16 << " != " << sizeof(sf_tag_msg_t) << ")");
				continue;
			}
			break;
		default:
			WARN(TAG,"unknown msg type");
			continue;
		}

		processor->dump(rxts, (uint8_t*)buffer, length_u16);
	}

	processor->end();

	return 0;
}
