# 젯슨에서 직접 빌드. 사용: make  /  make clean
# (모듈 빌드 규칙 줄은 반드시 TAB으로 들여쓰기)
# obj-m += led_b_dev.o
# obj-m += led_b.o
obj-m += bmp180.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all: modules bmp180_test #led_test

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

#led_test: led_test.c fan_common.h
#	$(CC) -Wall -o $@ led_test.c

bmp180_test: bmp180_test.c fan_common.h
	$(CC) -Wall -o $@ bmp180_test.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f led_test bmp180_test