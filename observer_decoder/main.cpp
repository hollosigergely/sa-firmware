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


int main(int argc, char** argv)
{
	//int fd = open("/tmp/build-observer_decoder-GCC_Desktop-Default/a",O_RDONLY);//STDIN_FILENO;
	int fd = STDIN_FILENO;

	std::shared_ptr<MessageDump> processor(new OstreamMessageDump());
	if(argc == 2) {
		processor = make_shared<Aggregate>();
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
		int length = (int)length_u16 - 2;
		if(length <= 0)
		{
			WARN(TAG,"length <= 0");
			continue;
		}

		dwm1000_ts_t rxts;
		myread(fd, &rxts, sizeof(uint64_t));
		myread(fd, buffer, length - 2);

		sf_header_t* hdr = (sf_header_t*)buffer;
		switch(hdr->fctrl)
		{
		case SF_HEADER_FCTRL_MSG_TYPE_ANCHOR_MESSAGE:
			if(length != sizeof(sf_anchor_msg_t))
			{
				WARN(TAG,"anchor message length warn (" << length << " != " << sizeof(sf_anchor_msg_t) << ")");
				continue;
			}
			break;
		case SF_HEADER_FCTRL_MSG_TYPE_TAG_MESSAGE:
			if(length != sizeof(sf_tag_msg_t))
			{
				WARN(TAG,"anchor message length warn (" << length << " != " << sizeof(sf_tag_msg_t) << ")");
				continue;
			}
			break;
		default:
			WARN(TAG,"unknown msg type");
			continue;
		}

		processor->dump(rxts, (uint8_t*)buffer, length);
	}

	processor->end();

	return 0;
}
