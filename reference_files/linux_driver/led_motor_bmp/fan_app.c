
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>

#include "fan_motor.h"
// #include "fnd_driver.h"

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

#define MOTOR_DEVICE "/dev/fan_motor"
#define FND_DEVICE   "/dev/fnd"

/* virtual_temp.c에서 검증한 AUTO 모드용 가상 온도 시나리오 */
static const float virtual_temperatures[] = {
    25.2f, 25.9f, 26.7f, 27.6f,
    27.0f, 26.2f, 25.4f
};

/* Application 종료 요청 */
static volatile sig_atomic_t stop_requested = 0;

static void signal_handler(int sig)
{
    (void)sig;
    stop_requested = 1;
}

/* 두 드라이버 모두 write()로 uint32_t 값 하나를 받는다. */
static int write_value(int fd, uint32_t value, const char *error_message)
{
    ssize_t ret = write(fd, &value, sizeof(value));

    if (ret != sizeof(value)) {
        if (ret >= 0)
            errno = EIO;

        perror(error_message);
        return -1;
    }

    return 0;
}

/* FND가 보낸 신호(시간 남음=0, 만료=1)를 받는다. */
static int fnd_get_timer_signal(int fd, uint32_t *signal)
{
    ssize_t ret;

    ret = read(fd, signal, sizeof(*signal));

    if (ret != sizeof(*signal)) {
        if (ret >= 0)
            errno = EIO;

        perror("fnd read");
        return -1;
    }

    return 0;
}

/* virtual_temp.c에서 검증한 온도 구간과 duty 값을 그대로 사용한다. */
static int temperature_to_duty(float temperature)
{
    if (temperature < 25.0f || temperature > 28.0f)
        return -1;

    if (temperature < 25.75f)
        return 60;
    if (temperature < 26.50f)
        return 80;
    if (temperature < 27.25f)
        return 99;

    return 100;
}

static int set_auto_duty(int motor_fd, unsigned int index)
{
    float temperature = virtual_temperatures[index];
    int calculated_duty = temperature_to_duty(temperature);
    uint32_t duty;

    if (calculated_duty < 0) {
        fprintf(stderr, "Invalid temperature: %.2f\n", temperature);
        return -1;
    }

    duty = (uint32_t)calculated_duty;
    if (ioctl(motor_fd, FAN_IOC_SET_DUTY, &duty) < 0) {
        perror("ioctl FAN_IOC_SET_DUTY");
        return -1;
    }

    printf("\nTemp: %.2f C -> Duty: %u%%\n", temperature, duty);
    return 0;
}

int main(int argc, char *argv[])
{
    int motor_fd = -1;
    int fnd_fd = -1;
    int result = EXIT_FAILURE;
    uint32_t mode;
    uint32_t minutes;
    uint32_t timer_signal;
    uint32_t remaining;
    unsigned int temperature_index = 0;
    unsigned int temperature_seconds = 0;
    char *end;
    unsigned long value;

    if (argc != 3) {
        fprintf(stderr,
                "Usage: %s <motor_mode 1~5> <minutes 1~99>\n",
                argv[0]);
        return EXIT_FAILURE;
    }

    /* 첫 번째 인자: 모터 모드 */
    errno = 0;
    value = strtoul(argv[1], &end, 10);

    if (errno != 0 || end == argv[1] ||
        *end != '\0' || argv[1][0] == '-' ||
        value < FAN_AUTO || value > FAN_FINE) {
        fprintf(stderr, "Invalid motor mode (1~5)\n");
        return EXIT_FAILURE;
    }

    mode = (uint32_t)value;

    /* 두 번째 인자: 타이머 분 */
    errno = 0;
    value = strtoul(argv[2], &end, 10);

    if (errno != 0 || end == argv[2] ||
        *end != '\0' || argv[2][0] == '-' ||
        value < 1 || value > 99) {
        fprintf(stderr, "Invalid timer (1~99 minutes)\n");
        return EXIT_FAILURE;
    }

    minutes = (uint32_t)value;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 1. 장치 파일 열기 */
    motor_fd = open(MOTOR_DEVICE, O_RDWR);

    if (motor_fd < 0) {
        perror("open motor");
        goto cleanup;
    }

    fnd_fd = open(FND_DEVICE, O_RDWR);

    if (fnd_fd < 0) {
        perror("open fnd");
        goto cleanup;
    }

    /* 2. FND 타이머 초기화 */
    if (ioctl(fnd_fd, FND_IOC_RESET) < 0) {
        perror("fnd reset");
        goto cleanup;
    }

    /* 3. 타이머 분 설정 */
    if (write_value(fnd_fd, minutes, "fnd write") < 0)
        goto cleanup;

    /* 4. Motor Driver 모드 설정 */
    if (write_value(motor_fd, mode, "motor write") < 0)
        goto cleanup;

    /* AUTO 모드는 첫 가상 온도에 맞춰 duty를 바로 설정한다. */
    if (mode == FAN_AUTO) {
        if (set_auto_duty(motor_fd, temperature_index) < 0)
            goto cleanup;

        temperature_index++;
    }

    /* 5. 타이머 시작 */
    if (ioctl(fnd_fd, FND_IOC_START) < 0) {
        perror("fnd start");
        goto cleanup;
    }

    printf("Motor mode  : %u\n", mode);
    printf("Timer       : %u minute(s)\n", minutes);
    printf("Timer started. Press Ctrl+C to stop.\n");

    /* 6. 타이머 만료 상태 감시 */
    while (!stop_requested) {

        if (fnd_get_timer_signal(fnd_fd, &timer_signal) < 0)
            goto cleanup;

        if (timer_signal == FND_TIMER_EXPIRED) {
            printf("\nTimer expired!\n");

            /* 신호 1을 받은 앱 동작 예: 현재는 모터를 끈다. */
            if (write_value(motor_fd, FAN_OFF, "motor write") < 0)
                goto cleanup;

            printf("Motor OFF (mode=0)\n");
            result = EXIT_SUCCESS;
            goto cleanup;
        }

        if (ioctl(fnd_fd, FND_IOC_GET_REMAINING,
                  &remaining) < 0) {
            perror("get remaining");
            goto cleanup;
        }

        printf("\rRemaining: %3u sec | Timer Signal: %u",
               remaining, timer_signal);
        fflush(stdout);

        sleep(1);

        /* AUTO 모드에서는 3초마다 다음 가상 온도로 duty를 갱신한다. */
        if (mode == FAN_AUTO &&
            temperature_index < sizeof(virtual_temperatures) /
                                sizeof(virtual_temperatures[0]) &&
            ++temperature_seconds == 3) {
            temperature_seconds = 0;

            if (set_auto_duty(motor_fd, temperature_index) < 0)
                goto cleanup;

            temperature_index++;
        }
    }

    printf("\nUser requested stop.\n");
    result = EXIT_SUCCESS;

cleanup:
    /*
     * 정상 종료, Ctrl+C, 오류 발생 시에도
     * 모터를 정지하도록 시도한다.
     */
    if (motor_fd >= 0 &&
        write_value(motor_fd, FAN_OFF, "motor write") < 0)
        result = EXIT_FAILURE;

    if (fnd_fd >= 0) {
        if (ioctl(fnd_fd, FND_IOC_RESET) < 0) {
            perror("fnd reset");
            result = EXIT_FAILURE;
        }
        close(fnd_fd);
    }

    if (motor_fd >= 0)
        close(motor_fd);

    printf("\nApplication terminated.\n");

    return result;
}
