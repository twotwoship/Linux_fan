#ifndef FND_DRIVER_H
#define FND_DRIVER_H

#include <linux/ioctl.h>
#include <linux/types.h>

enum encoder_rotation {
    CW  = 1,
    CCW = 2
};

int fnd_notify_rotation(enum encoder_rotation rotation);

#define FND_IOC_MAGIC 'F'
#define FND_IOC_START _IO(FND_IOC_MAGIC, 1)
#define FND_IOC_STOP  _IO(FND_IOC_MAGIC, 2)
#define FND_IOC_RESET _IO(FND_IOC_MAGIC, 3)
#define FND_IOC_GET_REMAINING _IOR(FND_IOC_MAGIC, 4, __u32)

#endif
