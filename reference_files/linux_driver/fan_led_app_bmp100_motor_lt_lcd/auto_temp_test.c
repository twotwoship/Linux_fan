#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "fan_motor.h"

#define MOTOR_DEVICE "/dev/fan_motor"

/* 온도 -> duty 변환 */
static int temperature_to_duty(float temperature)
{
    if (temperature < 25.0f || temperature > 28.0f)
        return -1;

    if (temperature < 25.75f)
        return 60;   // 2단계 LOW

    if (temperature < 26.50f)
        return 80;   // 3단계 MEDIUM

    if (temperature < 27.25f)
        return 90;   // 4단계 HIGH

    return 100;      // 5단계 FINE
}

/* 온도 -> 단계 표시용 */
static int temperature_to_stage(float temperature)
{
    if (temperature < 25.0f || temperature > 28.0f)
        return -1;

    if (temperature < 25.75f)
        return 2;

    if (temperature < 26.50f)
        return 3;

    if (temperature < 27.25f)
        return 4;

    return 5;
}

int main(void)
{
    int fd;
    uint32_t mode = FAN_AUTO;
    uint32_t duty;
    struct fan_status status;

    float temperatures[] = {
        25.2f,
        25.9f,
        26.7f,
        27.6f,
        27.0f,
        26.2f,
        25.4f
    };

    int count = sizeof(temperatures) / sizeof(temperatures[0]);
    int i;

    fd = open(MOTOR_DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* 1) 먼저 AUTO 모드 진입 */
    if (write(fd, &mode, sizeof(mode)) != sizeof(mode)) {
        perror("write AUTO mode");
        close(fd);
        return 1;
    }

    printf("AUTO mode start\n");

    /* 2) 가상 온도 시나리오 반복 */
    for (i = 0; i < count; i++) {
        float temp = temperatures[i];
        int stage = temperature_to_stage(temp);
        int calc_duty = temperature_to_duty(temp);

        if (calc_duty < 0) {
            printf("Invalid temperature: %.2f\n", temp);
            continue;
        }

        duty = (uint32_t)calc_duty;

        /* 3) AUTO duty 변경 */
        if (ioctl(fd, FAN_IOC_SET_DUTY, &duty) < 0) {
            perror("ioctl FAN_IOC_SET_DUTY");
            close(fd);
            return 1;
        }

        /* 4) 현재 상태 읽기 */
        if (read(fd, &status, sizeof(status)) != sizeof(status)) {
            perror("read");
            close(fd);
            return 1;
        }

        printf("[Time %2d sec] Temp: %.2f C -> Stage: %d -> Duty set: %u%% | Driver mode: %u duty: %u%%\n",
               i * 3, temp, stage, duty, status.mode, status.duty);

        sleep(3);
    }

    /* 5) 종료 시 모터 OFF */
    mode = FAN_OFF;
    if (write(fd, &mode, sizeof(mode)) != sizeof(mode)) {
        perror("write OFF");
        close(fd);
        return 1;
    }

    printf("AUTO mode end -> Motor OFF\n");

    close(fd);
    return 0;
}