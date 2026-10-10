#ifndef FAN_MOTOR_H
#define FAN_MOTOR_H

#include <linux/types.h>
#include <linux/ioctl.h>

enum fan_mode {
    FAN_OFF    = 0,
    FAN_AUTO   = 1,
    FAN_LOW    = 2,
    FAN_MEDIUM = 3,
    FAN_HIGH   = 4,
    FAN_FINE   = 5
};

struct fan_status {
    __u32 mode;
    __u32 duty;   // 0 ~ 100 (%)
};

#define FAN_IOC_MAGIC 'F'
#define FAN_IOC_SET_DUTY _IOW(FAN_IOC_MAGIC, 1, __u32)
#define FAN_IOC_GET_DUTY _IOR(FAN_IOC_MAGIC, 2, __u32)

#endif