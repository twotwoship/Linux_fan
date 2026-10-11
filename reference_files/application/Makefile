ifneq ($(KERNELRELEASE),)

# 7개 커널 모듈
obj-m += lt_en.o
obj-m += mt_d.o
obj-m += timer_f.o
obj-m += lcd_t.o
obj-m += buzz_t.o
obj-m += bmp180.o
obj-m += led_b.o

else

.DEFAULT_GOAL := all

# 현재 실행 중인 Jetson 커널의 빌드 경로
KDIR ?= /lib/modules/$(shell uname -r)/build
# 프로젝트 폴더의 절대 경로
MODULE_DIR := $(CURDIR)
# 사용자 프로그램 컴파일 설정
APP_CC ?= gcc
APP_CFLAGS ?= -std=c11 -Wall -Wextra -O2

.PHONY: all modules clean
# 기본: 7개 모듈과 앱 모두 빌드
all: modules app
# 커널 모듈 빌드
modules:
	$(MAKE) -C "$(KDIR)" M="$(MODULE_DIR)" modules

# 사용자 프로그램 빌드
# app에서 사용하는 헤더를 의존성으로 명시
app: app.c fan_motor.h
	$(APP_CC) $(APP_CFLAGS) -o $@ app.c

# 빌드 결과 삭제
clean:
	$(MAKE) -C "$(KDIR)" M="$(MODULE_DIR)" clean
	$(RM) app

endif
