#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#define DEV_NAME "/dev/lt_en"

int main(void)
{
    int fd;
    int ret;
    int value;

    struct pollfd pfd;

    fd = open(DEV_NAME, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    pfd.fd = fd;
    pfd.events = POLLIN;

    while (1) {
        /*
         * 최대 100ms 대기
         * KEY가 들어오면 즉시 반환
         * 안 들어와도 100ms 후 반환
         */
        ret = poll(&pfd, 1, 100);

        if (ret < 0) {
            perror("poll");
            break;
        }

        if (ret > 0 && (pfd.revents & POLLIN)) {
            ret = read(fd, &value, sizeof(value));
            if (ret == sizeof(value)) {
                if (value == 1) {
                    printf("KEY pressed\n");

                    /*
                     * 여기에 버튼 눌렀을 때
                     * 할 일을 작성
                     */
                }
            }
        }

        /*
         * 다른 작업
         *
         * LCD 처리
         * FND 처리
         * 팬 상태 처리
         * 타이머 처리
         * 등
         */
    }

    close(fd);

    return 0;
}