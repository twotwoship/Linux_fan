
# ==========================================
# Kernel Modules
# ==========================================

obj-m += lt_en.o
obj-m += fan_motor.o
obj-m += fnd_driver.o
obj-m += bmp180.o

# LED Driver
obj-m += led_b_dev.o
obj-m += led_b.o


# ==========================================
# Kernel Build Directory
# ==========================================

KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)


# ==========================================
# Application Programs
# ==========================================

APPS := apptest fan_test fnd_test fan_app manual_test bmp180_test


# ==========================================
# Build
# ==========================================

.PHONY: all default modules clean

all: modules $(APPS)

default: all

# Kernel Module Build
modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

# Application Build
$(APPS): %: %.c
	$(CC) -Wall -Wextra -o $@ $<


# ==========================================
# Clean
# ==========================================

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
	rm -f $(APPS)
