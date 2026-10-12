#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "fan_motor.h"
#include "fnd_driver.h"

#define MOTOR_DEVICE "/dev/fan_motor"
#define ENCODER_DEVICE "/dev/lt_en"
#define FND_DEVICE "/dev/fnd"
#define BUZZ_DEVICE "/dev/buzz_t"

static volatile sig_atomic_t stopping = 0;

static void stop_app(int signo)
{
    (void)signo;
    stopping = 1;
}

/* fan_motor.c write() receives a 4-byte mode. */
static int motor_send_mode(int fd, enum fan_mode mode)
{
    __u32 command = (__u32)mode;
    ssize_t written;

    do {
        written = write(fd, &command, sizeof(command));
    } while (written < 0 && errno == EINTR);

    if (written < 0)
        return -1;
    if (written != (ssize_t)sizeof(command)) {
        errno = EIO;
        return -1;
    }
    return 0;
}

/* OFF -> AUTO -> LOW -> MEDIUM -> HIGH -> FINE -> OFF */
static enum fan_mode next_mode(enum fan_mode current)
{
    if (current == FAN_HIGH)
        return FAN_OFF;
    return (enum fan_mode)(current + 1);
}

static int buzzer_play(int fd){
    int value = 1;
    ssize_t written;

    do{
        written = write(fd,&value,sizeof(value));
    }while (written < 0 && errno == EINTR);

    if(written < 0) return -1;

    if(written != sizeof(value)){
        errno = EIO;
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    const char *motor_path = MOTOR_DEVICE;
    struct sigaction action = { 0 };
    struct pollfd button = { 0 };
    enum fan_mode mode = FAN_OFF;
    int rotary_fd = -1;
    int motor_fd = -1;
    int fnd_fd = -1;
    int buzz_fd = -1;
    int status = EXIT_SUCCESS;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [MOTOR_DEVICE]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2)
        motor_path = argv[1];

    action.sa_handler = stop_app;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGINT, &action, NULL) < 0 ||
        sigaction(SIGTERM, &action, NULL) < 0) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    /* KEY events are delivered through read()/poll().
     * CW/CCW events go directly from lt_en to fnd_driver via kernel FIFO.
     */
    rotary_fd = open(ENCODER_DEVICE, O_RDONLY | O_CLOEXEC);
    if (rotary_fd < 0) {
        perror("open /dev/lt_en");
        status = EXIT_FAILURE;
        goto cleanup;
    }

    motor_fd = open(motor_path, O_WRONLY | O_CLOEXEC);
    if (motor_fd < 0) {
        perror("open fan_motor");
        status = EXIT_FAILURE;
        goto cleanup;
    }

    fnd_fd = open(FND_DEVICE, O_RDWR | O_CLOEXEC);
    if (fnd_fd < 0) {
        perror("open /dev/fnd");
        status = EXIT_FAILURE;
        goto cleanup;
    }

    buzz_fd = open(BUZZ_DEVICE, O_WRONLY | O_CLOEXEC);
    if (buzz_fd < 0) {
        perror("open /dev/buzz_t");
        status = EXIT_FAILURE;
        goto cleanup;
    }

    /* Start in OFF, with the FND lit and the timer reset to 00. */
    if (motor_send_mode(motor_fd, FAN_OFF) < 0) {
        perror("motor initial OFF");
        status = EXIT_FAILURE;
        goto cleanup;
    }
    if (ioctl(fnd_fd, FND_IOC_RESET) < 0 ||
        ioctl(fnd_fd, FND_IOC_DISPLAY_ON) < 0) {
        perror("FND initialization");
        status = EXIT_FAILURE;
        goto cleanup;
    }

    printf("mode=%d (OFF) | KEY: mode change, CW/CCW: timer +/-1 min\n",
           (int)mode);

    button.fd = rotary_fd;
    button.events = POLLIN;

    while (!stopping) {
        int result;

        /* Timeout also lets the APP check FND expiry without a KEY press. */
        result = poll(&button, 1, 200);
        if (result < 0) {
            if (errno == EINTR)
                continue;
            perror("poll lt_en");
            status = EXIT_FAILURE;
            break;
        }
        if (stopping)
            break;

        if (button.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr, "lt_en device error: 0x%x\n",
                    (unsigned int)button.revents);
            status = EXIT_FAILURE;
            break;
        }

        if (result > 0 && (button.revents & POLLIN)) {
            int key_event;
            ssize_t received;
            enum fan_mode next;

            received = read(rotary_fd, &key_event, sizeof(key_event));
            if (received < 0) {
                if (errno == EINTR || errno == EAGAIN)
                    continue;
                perror("read lt_en");
                status = EXIT_FAILURE;
                break;
            }
            if (received != (ssize_t)sizeof(key_event)) {
                fprintf(stderr, "lt_en: incomplete KEY event\n");
                status = EXIT_FAILURE;
                break;
            }
            if (key_event != 1)
                continue;

            next = next_mode(mode);
            if (motor_send_mode(motor_fd, next) < 0) {
                perror("motor mode write");
                status = EXIT_FAILURE;
                break;
            }

            /* AUTO: FND fully blank + countdown reset.
             * All other modes: show FND. OFF pauses any countdown.
             */
            if (next == FAN_AUTO) {
                if (ioctl(fnd_fd, FND_IOC_DISPLAY_OFF) < 0) {
                    perror("FND display OFF");
                    status = EXIT_FAILURE;
                    break;
                }
            } else {
                if (ioctl(fnd_fd, FND_IOC_DISPLAY_ON) < 0) {
                    perror("FND display ON");
                    status = EXIT_FAILURE;
                    break;
                }
                if (next == FAN_OFF &&
                    ioctl(fnd_fd, FND_IOC_STOP) < 0) {
                    perror("FND timer STOP");
                    status = EXIT_FAILURE;
                    break;
                }
            }

            mode = next;
            printf("mode=%d%s\n", (int)mode,
                   mode == FAN_AUTO ? " (FND OFF)" : "");
        }

        if (mode >= FAN_LOW && mode <= FAN_FINE) {
            __u32 timer_signal;
            __u32 remaining_sec;
            ssize_t received;

            received = read(fnd_fd, &timer_signal, sizeof(timer_signal));
            if (received != (ssize_t)sizeof(timer_signal)) {
                if (received >= 0)
                    errno = EIO;
                perror("read FND timer signal");
                status = EXIT_FAILURE;
                break;
            }

            if (timer_signal == FND_TIMER_EXPIRED) {
                if (motor_send_mode(motor_fd, FAN_OFF) < 0){
                    perror("motor OFF");
                    status = EXIT_FAILURE;
                    break;
                }

                mode = FAN_OFF;

                if(ioctl(fnd_fd, FND_IOC_RESET) < 0) {
                    perror("FND RESET");
                    status = EXIT_FAILURE;
                    break;
                }
                if(buzzer_play(buzz_fd) <0){
                    perror("buzzer play");
                    status = EXIT_FAILURE;
                    break;
                }
            
                printf("FND timer expired -> motor OFF, Buzzer ON(mode=0)\n");
                continue;
            }

            if (ioctl(fnd_fd, FND_IOC_GET_REMAINING,
                      &remaining_sec) < 0) {
                perror("FND GET_REMAINING");
                status = EXIT_FAILURE;
                break;
            }

            /* Starts as soon as CW produces a nonzero time in manual mode.
             * FND_IOC_START is harmless when timer is already running.
             */
            if (remaining_sec > 0 &&
                ioctl(fnd_fd, FND_IOC_START) < 0 && errno != EINVAL) {
                perror("FND START");
                status = EXIT_FAILURE;
                break;
            }
        }
    }

cleanup:
    if (motor_fd >= 0) {
        if (motor_send_mode(motor_fd, FAN_OFF) < 0) {
            perror("motor shutdown OFF");
            status = EXIT_FAILURE;
        }
        close(motor_fd);
    }
    if (fnd_fd >= 0) {
        if (ioctl(fnd_fd, FND_IOC_DISPLAY_OFF) < 0) {
            perror("FND shutdown OFF");
            status = EXIT_FAILURE;
        }
        close(fnd_fd);
    }
    if (rotary_fd >= 0)
        close(rotary_fd);

    if (buzz_fd >= 0)
        close(buzz_fd);

    return status;
}
