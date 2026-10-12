ifneq ($(KERNELRELEASE),)

# buzz_t.c → buzz_t.ko
obj-m := buzz_t.o

else

.DEFAULT_GOAL := all

KDIR ?= /lib/modules/$(shell uname -r)/build
MODULE_DIR := $(CURDIR)

APP_CC ?= gcc
APP_CFLAGS ?= -std=c11 -Wall -Wextra -O2

.PHONY: all modules clean

# 부저 모듈과 테스트 프로그램 빌드
all: modules buzz_t

modules:
	$(MAKE) -C "$(KDIR)" M="$(MODULE_DIR)" modules

# buzzer_test.c → 실행 파일 buzzer_t
buzz_t: buzz_t_test.c
	$(APP_CC) $(APP_CFLAGS) -o $@ $<

clean:
	$(MAKE) -C "$(KDIR)" M="$(MODULE_DIR)" clean
	$(RM) buzz_t

endif