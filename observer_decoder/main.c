#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdint.h>
#include <inttypes.h>
#include <byteswap.h>

static char buffer[65536];

void myread (int __fd, void *__buf, size_t __nbytes)
{
	do {
		int ret = read(__fd, __buf, __nbytes);
		__nbytes -= ret;
		__buf += ret;
	} while(__nbytes != 0);
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

		printf("%d %"PRIx64" %02.5f ",
			   length - 2,
			   rxts,
			   (double)rxts / (499.2e6 * 128));
		for(int i = 0; i < length - 2; i++)
		{
			printf("%02X", buffer[i]);
		}

		printf("\n");
	}

	return 0;
}
