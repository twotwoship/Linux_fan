#ifndef FND_DRIVER_H
#define FND_DRIVER_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define FND_IOC_MAGIC 'F'
#define FND_IOC_START         _IO(FND_IOC_MAGIC, 1)
#define FND_IOC_STOP          _IO(FND_IOC_MAGIC, 2)
#define FND_IOC_RESET         _IO(FND_IOC_MAGIC, 3)
#define FND_IOC_GET_REMAINING _IOR(FND_IOC_MAGIC, 4, __u32)
#define FND_IOC_DISPLAY_OFF   _IO(FND_IOC_MAGIC, 5)
#define FND_IOC_DISPLAY_ON    _IO(FND_IOC_MAGIC, 6)

#define FND_TIMER_RUNNING 0
#define FND_TIMER_EXPIRED 1

#endif
