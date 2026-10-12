#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "fan_motor.h"
#include "fnd_driver.h"

#define FND_DEVICE     "/dev/fnd"
#define MOTOR_DEVICE   "/dev/fan_motor"
#define ENCODER_DEVICE "/dev/lt_en"

static volatile sig_atomic_t stop_requested;

static void stop_handler(int signal)
{
    (void)signal;
    stop_requested = 1;
}

static int write_u32(int fd, uint32_t value)
{
    return write(fd, &value, sizeof(value)) == sizeof(value) ? 0 : -1;
}

static uint32_t next_manual_mode(uint32_t mode)
{
    return mode == FAN_HIGH ? FAN_FINE : mode + 1;
}

int main(int argc, char *argv[])
{
    int fnd_fd = -1, motor_fd = -1, encoder_fd = -1;
    uint32_t mode, timer_signal;
    char command[16];
    struct pollfd encoder_poll;
    int result = EXIT_FAILURE;

    if (argc != 2 || sscanf(argv[1], "%u", &mode) != 1 ||
        mode < FAN_FINE || mode > FAN_HIGH) {
        fprintf(stderr, "Usage: %s <manual motor mode 2~5>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fnd_fd = open(FND_DEVICE, O_RDWR);
    motor_fd = open(MOTOR_DEVICE, O_RDWR);
    encoder_fd = open(ENCODER_DEVICE, O_RDONLY);
    if (fnd_fd < 0 || motor_fd < 0 || encoder_fd < 0) {
        perror("open device");
        goto cleanup;
    }

    if (ioctl(fnd_fd, FND_IOC_RESET) < 0) {
        perror("fnd reset");
        goto cleanup;
    }

    puts("Set the time with the physical encoder, then enter start or quit.");

    while (printf("> "), fflush(stdout), scanf("%15s", command) == 1) {
        if (!strcmp(command, "start"))
            break;
        else if (!strcmp(command, "quit")) {
            result = EXIT_SUCCESS;
            goto cleanup;
        } else {
            puts("Use: start | quit");
            continue;
        }
    }

    if (write_u32(motor_fd, mode) || ioctl(fnd_fd, FND_IOC_START) < 0) {
        perror("start");
        goto cleanup;
    }

    puts("Motor started. Ctrl+C stops it.");
    puts("Each physical KEY press selects the next manual motor mode.");
    signal(SIGINT, stop_handler);
    signal(SIGTERM, stop_handler);
    encoder_poll.fd = encoder_fd;
    encoder_poll.events = POLLIN;
    while (!stop_requested) {
        int key_event;
        int poll_ret;

        poll_ret = poll(&encoder_poll, 1, 1000);
        if (poll_ret < 0 && errno == EINTR)
            continue;
        if (poll_ret < 0) {
            perror("encoder poll");
            goto cleanup;
        }

        if (poll_ret > 0 && (encoder_poll.revents & POLLIN)) {
            if (read(encoder_fd, &key_event, sizeof(key_event)) !=
                sizeof(key_event)) {
                perror("encoder read");
                goto cleanup;
            }

            mode = next_manual_mode(mode);
            if (write_u32(motor_fd, mode)) {
                perror("motor mode change");
                goto cleanup;
            }
            printf("Motor mode: %u\n", mode);
        }

        if (read(fnd_fd, &timer_signal, sizeof(timer_signal)) != sizeof(timer_signal)) {
            perror("fnd read");
            goto cleanup;
        }
        if (timer_signal == 1) {
            puts("Timer expired; motor stopping.");
            break;
        }
        sleep(1);
    }
    result = EXIT_SUCCESS;

cleanup:
    if (motor_fd >= 0)
        write_u32(motor_fd, FAN_OFF);
    if (fnd_fd >= 0) {
        ioctl(fnd_fd, FND_IOC_RESET);
        close(fnd_fd);
    }
    if (motor_fd >= 0)
        close(motor_fd);
    if (encoder_fd >= 0)
        close(encoder_fd);
    return result;
}
