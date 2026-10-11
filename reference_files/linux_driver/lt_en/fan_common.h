#ifndef FAN_COMMON_H
#define FAN_COMMON_H

#include <linux/ioctl.h>

/* 풍량 값 (모터드라이버 담당자가 LED 드라이버로 넘기는 값) */
#define FAN_SPEED_OFF     0
#define FAN_SPEED_WEAK    25    /* 약풍 */
#define FAN_SPEED_MID     50    /* 중풍 */
#define FAN_SPEED_STRONG  75    /* 강풍 */
#define FAN_SPEED_BREEZE  100   /* 미풍 (값 확인 필요) */

/* LED BAR ioctl */
#define LED_IOC_MAGIC      'L'
#define LED_IOC_SET_LEVEL  _IOW(LED_IOC_MAGIC, 1, int)

/* BMP180 (온도/기압) : 정수만 사용 (커널에서 float 금지) */
struct bmp180_data {
	int temp_x10;      /* 0.1℃ 단위 (253 = 25.3℃) */
	int pressure_pa;   /* Pa 단위 (101325 = 1013.25 hPa) */
};

#define BMP180_IOC_MAGIC  'B'
#define BMP180_IOC_READ   _IOR(BMP180_IOC_MAGIC, 1, struct bmp180_data)

int bmp180_get_data(struct bmp180_data *out);

#define _CW 1
#define _CCW 2

int _cw_ccw_event(void);

#endif /* FAN_COMMON_H */