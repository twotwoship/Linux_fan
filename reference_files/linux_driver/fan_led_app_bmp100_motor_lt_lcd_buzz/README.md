# 📌 프로젝트 빌드 및 실행 가이드

이 문서는 모듈 빌드, 커널 모듈 로드 및 애플리케이션 실행 순서를 안내합니다.

---

## 1. 빌드 (Build)

기존 빌드 파일을 정리하고 새로 컴파일을 진행합니다.

```bash
make clean
make
```

LED duty는 `led_ctl.h`에서 `fan_common.h`의 모터 상수를 공유합니다.
미풍/약풍/중풍/강풍은 65/75/85/95%이고 LED는 각각 1/3/5/7개 켜집니다.
매핑 검증은 `python3 ../fan_led_app_bmp100_motor_lt_lcd/test_led_mapping.py`로 실행합니다.
기존 모듈이 로드되어 있다면 앱을 종료하고 `sudo rmmod fan_motor led_b led_b_dev`로
내린 뒤 새로 빌드한 LED 장치 → LED 드라이버 → 모터 순서로 다시 로드하세요.

sudo dmesg -w
---

## 2. 드라이버 모듈 로드 (Module Load)
 // 장치 확인

sudo insmod ./bmp180.ko bus=7
sudo insmod ./lt_en.ko
sudo insmod led_b_dev.ko gpios=458,459,401,400,483,482,481 active_low=0
sudo insmod led_b.ko 
sudo insmod ./fan_motor.ko
sudo insmod ./fnd_driver.ko
sudo insmod ./lcd_t.ko
sudo insmod ./buzz_t.ko
echo lcd_t 0x27 | sudo tee /sys/bus/i2c/devices/i2c-1/new_device 


sudo mknod /dev/lt_en c 488 0  
sudo mknod /dev/buzz_t c 485 0  

보통 488 아니면 489


## 3. 애플리케이션 실행 (Application Run)

모듈 로드가 완료되면 아래 명령어로 메인 애플리케이션을 실행합니다.

```bash
sudo ./fan_app
```


sudo rmmod ./lcd_t.ko
sudo rmmod ./fnd_driver.ko
sudo rmmod ./fan_motor.ko
sudo rmmod ./lt_en.ko
sudo rmmod ./bmp180.ko
sudo rmmod ./led_b.ko
sudo rmmod led_b_dev.ko
sudo rmmod ./buzz_t.ko
echo 0x27 | sudo tee /sys/bus/i2c/devices/i2c-1/delete_device
