/* bmp180_test.c : BMP180 테스트 앱.  사용: ./bmp180_test [횟수] */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "fan_common.h"

int main(int argc, char **argv)
{
	struct bmp180_data d;
	int fd, i, n = (argc > 1) ? atoi(argv[1]) : 1;

	fd = open("/dev/bmp180", O_RDWR);
	if (fd < 0) {
		perror("open /dev/bmp180");
		return EXIT_FAILURE;
	}
	for (i = 0; i < n; i++) {
		if (ioctl(fd, BMP180_IOC_READ, &d) < 0) {
			perror("ioctl");
			close(fd);
			return EXIT_FAILURE;
		}
		printf("temp = %s%d.%d C (raw x10 = %d), pressure = %d Pa (%d.%02d hPa)\n",
		       d.temp_x10 < 0 ? "-" : "", abs(d.temp_x10) / 10, abs(d.temp_x10) % 10,
		       d.temp_x10, d.pressure_pa,
		       d.pressure_pa / 100, d.pressure_pa % 100);
		if (i + 1 < n)
			sleep(1);
	}
	close(fd);
	return EXIT_SUCCESS;
}
