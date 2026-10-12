# 젯슨에서 직접 빌드. 사용: make  /  make clean
# (모듈 빌드 규칙 줄은 반드시 TAB으로 들여쓰기)
obj-m += led_b_dev.o
obj-m += led_b.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
