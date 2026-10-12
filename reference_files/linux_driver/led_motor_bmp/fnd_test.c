
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

//#include "fnd_driver.h"

#define DEVICE_NAME "fnd"
#define FND_GPIO_COUNT 9
#define REFRESH_MS 4

#define FND_DRIVER_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define FND_IOC_MAGIC 'F'

#define FND_IOC_START _IO(FND_IOC_MAGIC, 1)
#define FND_IOC_STOP  _IO(FND_IOC_MAGIC, 2)
#define FND_IOC_RESET _IO(FND_IOC_MAGIC, 3)

/* 남은 초 조회: __u32 * */
#define FND_IOC_GET_REMAINING _IOR(FND_IOC_MAGIC, 4, __u32)

/* read()가 앱에 전달하는 타이머 신호 */
#define FND_TIMER_RUNNING 0
#define FND_TIMER_EXPIRED 1

int main(int argc, char *argv[])
{
    int fd;
    int ret = 0;
    int result = EXIT_FAILURE;
    uint32_t value, signal, remaining;
    char *end;
    unsigned long input;

    if (argc != 2) {
        printf("Usage: %s <0~99|start|stop|reset|status>\n",
               argv[0]);
        return 1;
    }

    fd = open("/dev/fnd", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (!strcmp(argv[1], "start")) {
        ret = ioctl(fd, FND_IOC_START);
    }
    else if (!strcmp(argv[1], "stop")) {
        ret = ioctl(fd, FND_IOC_STOP);
    }
    else if (!strcmp(argv[1], "reset")) {
        ret = ioctl(fd, FND_IOC_RESET);
    }
    else if (!strcmp(argv[1], "status")) {
        if (read(fd, &signal, sizeof(signal))
                != sizeof(signal)) {
            perror("read");
            goto cleanup;
        }

        if (ioctl(fd, FND_IOC_GET_REMAINING,
                  &remaining) < 0) {
            perror("ioctl GET_REMAINING");
            goto cleanup;
        }

        printf("Timer Signal : %u\n", signal);
        printf("Remaining  : %u sec\n", remaining);

        result = EXIT_SUCCESS;
        goto cleanup;
    }
    else {
        errno = 0;
        input = strtoul(argv[1], &end, 10);

        if (errno != 0 || end == argv[1] ||
            *end != '\0' || argv[1][0] == '-' ||
            input > 99) {
            printf("Invalid value\n");
            goto cleanup;
        }

        value = (uint32_t)input;

        if (write(fd, &value, sizeof(value))
                != sizeof(value)) {
            perror("write");
            goto cleanup;
        }

        printf("Timer Set: %u min\n", value);
        result = EXIT_SUCCESS;
        goto cleanup;
    }

    if (ret < 0) {
        perror("ioctl");
        goto cleanup;
    }

    printf("Command OK: %s\n", argv[1]);
    result = EXIT_SUCCESS;

cleanup:
    close(fd);
    return result;
}
