#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    const char* device = "/dev/buzz_t";
    int fd;
    int value = 1;
    ssize_t written;

    /* 장치 경로는 선택적으로 지정 가능 */
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [BUZZER_DEVICE]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (argc == 2)
        device = argv[1];

    /* 부저 장치 열기 */
    fd = open(device, O_WRONLY);
    if (fd < 0) {
        perror("open buzzer");
        return EXIT_FAILURE;
    }

    /* 문자열 '1'이 아니라 정수 1을 전달 */
    do {
        written = write(fd, &value, sizeof(value));
    } while (written < 0 && errno == EINTR);

    if (written < 0) {
        perror("write buzzer");
        close(fd);
        return EXIT_FAILURE;
    }

    if (written != (ssize_t)sizeof(value)) {
        fprintf(stderr, "buzzer: incomplete write\n");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("부저 알림음 재생을 요청했습니다.\n");

    close(fd);
    return EXIT_SUCCESS;
}
