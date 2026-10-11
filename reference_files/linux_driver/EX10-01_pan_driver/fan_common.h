#ifndef FAN_COMMON_H
#define FAN_COMMON_H
#include <linux/ioctl.h>
#define FAN_SPEED_OFF 0
#define FAN_SPEED_WEAK 25
#define FAN_SPEED_MID 50
#define FAN_SPEED_STRONG 75
#define FAN_SPEED_BREEZE 100
#define LED_IOC_MAGIC 'L'
#define LED_IOC_SET_LEVEL _IOW(LED_IOC_MAGIC, 1, int)

/* Kernel API exported by LED driver. Input: actual PWM duty (0..100%). */
int led_set_duty(unsigned int duty);
struct bmp180_data {
    int temp_x10;
    int pressure_pa;
};
#define BMP180_IOC_MAGIC 'B'
#define BMP180_IOC_READ _IOR(BMP180_IOC_MAGIC, 1, struct bmp180_data)
int bmp180_get_data(struct bmp180_data *out);
#endif
