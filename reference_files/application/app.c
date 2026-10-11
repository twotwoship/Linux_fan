#define _POSIX_C_SOURCE 200809L //sigaction(), O_CLOEXEC 등 프로그램에서 사용하는 POSIX 기능을 사용

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fan_motor.h"

static volatile sig_atomic_t stopping = 0; //fnd에서 받는 stop신호
/*
enum fan_mode {
    FAN_OFF = 0,
    FAN_AUTO = 1,
    FAN_LOW = 2,
    FAN_MEDIUM = 3,
    FAN_HIGH = 4,
    FAN_FINE = 5
};
*/
static void stop_app(int signo) 
{
    (void)signo;
    stopping = 1;
}

//모터 드라이버에 모드를 전달하는 것
//fan_motor.c write 요구 형식 __u32에 맞춤
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

//다음 fan_mode 설정(OFF->AUTO->LOW->MEDIUM->HIGH->FINE->OFF)
static enum fan_mode next_mode(enum fan_mode current)
{
    if (current == FAN_FINE)
        return FAN_OFF;

    return (enum fan_mode)(current + 1);
}

int main(int argc, char **argv)
{
    const char* motor_path = "/dev/mt_d"; // 지금 fan_motor로 되어 있음
    struct sigaction action = { 0 }; 
    struct pollfd button = { 0 }; 
    
    enum fan_mode mode = FAN_OFF; 

    int rotary_fd = -1;
    int motor_fd = -1;
    int status = EXIT_SUCCESS; 

    if (argc > 2) {
        fprintf(stderr,
            "Usage: %s [MOTOR_DEVICE]\n",
            argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2)
        motor_path = argv[1];

    // Ctrl+C 및 종료 요청 처리 등록. 
    action.sa_handler = stop_app; 
    sigemptyset(&action.sa_mask); 

    if (sigaction(SIGINT, &action, NULL) < 0 || sigaction(SIGTERM, &action, NULL) < 0) { 
        perror("sigaction"); 
        return EXIT_FAILURE;
    }

    //로터리 장치 열기
    rotary_fd = open("/dev/lt_en", O_RDONLY | O_CLOEXEC); //읽기 전용으로 하고 다른 프로그램 교체할 때 열어 둔 창치를 자동으로 닫기 위한 옵션
    if (rotary_fd < 0) {
        perror("open /dev/lt_en");
        status = EXIT_FAILURE;
        goto cleanup; //아래 cleanup으로 정리
    } 
    
    //모터 파일 쓰기 열기
    motor_fd = open(motor_path, O_WRONLY | O_CLOEXEC);
    if (motor_fd < 0) {
        perror("open fan_motor");
        status = EXIT_FAILURE;
        goto cleanup;
    }

    // 앱 시작 시 실제 모터도 OFF 상태로 맞춘다.
    if (motor_send_mode(motor_fd, FAN_OFF) < 0) {
        perror("motor initial OFF");
        status = EXIT_FAILURE;
        goto cleanup;
    }

    printf("mode=%d\n", (int)mode);


    // 로터리 장치 감시하고, 읽을 데이터가 생긴지 확인
    button.fd = rotary_fd; 
    button.events = POLLIN;

    while (!stopping) {
        int result;
        int key_event;
        ssize_t received;
        enum fan_mode next;

        
        // Key event를 최대 200ms마다 버튼 기달림
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

        // 장치 오류 확인 revents는 여러 이벤트 들어감 오류/연결 종류 등 상태/유효하지 않은 장치 번호
        if (button.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            fprintf(stderr,"lt_en device error: 0x%x\n",(unsigned int)button.revents);
            status = EXIT_FAILURE;
            break;
        }

        // 읽을 이벤트 없으면 다시 대기
        if (result == 0 || !(button.revents & POLLIN))
            continue;

        // key event 하나 읽기
        received = read(rotary_fd,&key_event,sizeof(key_event));

        if (received < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;

            perror("read lt_en");
            status = EXIT_FAILURE;
            break;
        }

        if (received != (ssize_t)sizeof(key_event)) {
            fprintf(stderr,
                "lt_en: incomplete KEY event\n");
            status = EXIT_FAILURE;
            break;
        }

        if (stopping)
            break;

        if (key_event != 1) {
            fprintf(stderr,
                "lt_en: invalid KEY event %d\n",
                key_event);
            status = EXIT_FAILURE;
            break;
        }

        // 버튼 한 번에 다음 모드로 변경
        next = next_mode(mode);

        if (motor_send_mode(motor_fd, next) < 0) {
            perror("motor mode write");
            status = EXIT_FAILURE;
            break;
        }
        mode = next;

        printf("mode=%d\n", (int)mode);
    }

cleanup:
    //clean_up 정리
    if (motor_fd >= 0) {
        if (motor_send_mode(motor_fd, FAN_OFF) < 0) {
            perror("motor shutdown OFF");
            status = EXIT_FAILURE;
        }

        close(motor_fd);
    }

    if (rotary_fd >= 0)
        close(rotary_fd);

    return status;
}