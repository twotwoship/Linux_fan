#ifndef LED_CTL_H
#define LED_CTL_H

/*
 * led_ctl.h : LED BAR로 들어오는 값의 약속 (led_b.c 와 APP/테스트 앱이 같이 include)
 *  - duty 값: 모터 드라이버(motor.c)가 실제 PWM에 쓰는 값. LED는 이 값을 칸 수로 바꿔 보여준다.
 *  - ioctl  : /dev/led_bar 로 같은 duty 값을 직접 줄 때 사용 (테스트/APP용)
 * 모터 쪽 fan_common.h 에도 같은 LED_IOC_* 와 led_set_duty() 선언이 있다. 정의가 같으므로 같이 include 해도 된다.
 */

#include <linux/ioctl.h>

/* ---- 모터가 쓰는 duty 값 (motor.c: FAN_LOW=70, FAN_MEDIUM=80, FAN_HIGH=90, FAN_FINE=100) ---- */
#define LED_DUTY_OFF     0
#define LED_DUTY_WEAK    70     /* 약풍 */
#define LED_DUTY_MID     80     /* 중풍 */
#define LED_DUTY_STRONG  90     /* 강풍 */
#define LED_DUTY_BREEZE  100    /* 미풍 (확정) */

/* ---- LED BAR ioctl: int duty 값 하나를 넘김 ---- */
#define LED_IOC_MAGIC      'L'
#define LED_IOC_SET_LEVEL  _IOW(LED_IOC_MAGIC, 1, int)

#ifdef __KERNEL__
/* 모터 드라이버가 호출하는 커널 API (led_b.ko 가 EXPORT_SYMBOL). 모터 쪽 fan_common.h 선언과 같아야 한다 */
int led_set_duty(unsigned int duty);
#endif

#endif /* LED_CTL_H */
