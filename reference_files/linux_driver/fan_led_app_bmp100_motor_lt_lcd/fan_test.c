
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "fan_motor.h"

int main(int argc, char *argv[])
{
    int fd;
    int result = EXIT_FAILURE;
    uint32_t mode;
    uint32_t duty;
    struct fan_status status;
    char *end;
    unsigned long value;

    if (argc < 2 || argc > 3) {
        printf("Usage: %s [0~5] [duty 0~100]\n", argv[0]);
        printf("Duty argument is allowed only in AUTO mode (1).\n");
        return 1;
    }

    errno = 0;
    value = strtoul(argv[1], &end, 10);

    if (errno != 0 || end == argv[1] ||
        *end != '\0' || value > FAN_HIGH) {
        printf("Invalid mode\n");
        return 1;
    }

    mode = (uint32_t)value;

    if ((argc == 3 && mode != FAN_AUTO) ||
        (argc == 2 && mode == FAN_AUTO)) {
        printf("AUTO mode requires a duty argument.\n");
        return 1;
    }

    if (argc == 3) {
        errno = 0;
        value = strtoul(argv[2], &end, 10);

        if (errno != 0 || end == argv[2] ||
            *end != '\0' || value > 100) {
            printf("Invalid duty\n");
            return 1;
        }

        duty = (uint32_t)value;
    }

    fd = open("/dev/fan_motor", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* 1. write(): 모드 설정 */
    if (write(fd, &mode, sizeof(mode)) != sizeof(mode)) {
        perror("write");
        goto cleanup;
    }

    /* 2. ioctl(): AUTO 모드 Duty 설정 */
    if (mode == FAN_AUTO && ioctl(fd, FAN_IOC_SET_DUTY, &duty) < 0) {
        perror("ioctl SET_DUTY");
        goto cleanup;
    }

    /* 3. read(): 현재 모터 상태 조회 */
    if (read(fd, &status, sizeof(status)) != sizeof(status)) {
        perror("read");
        goto cleanup;
    }

    printf("Motor Mode : %u\n", status.mode);
    printf("Motor Duty : %u%%\n", status.duty);

    /* 4. ioctl(): 현재 Duty 조회 */
    if (ioctl(fd, FAN_IOC_GET_DUTY, &duty) < 0) {
        perror("ioctl GET_DUTY");
        goto cleanup;
    }

    printf("ioctl Duty : %u%%\n", duty);
    result = EXIT_SUCCESS;

cleanup:
    close(fd);
    return result;
}
