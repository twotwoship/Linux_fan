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
검증은 두 폴더의 실제 모터 코드를 모두 검사합니다. 통합 모터의 수동 모드도 공통 상수를
사용해야 하므로 LED뿐 아니라 `fan_motor.ko`도 새로 빌드하고 교체하세요.
미풍에서 기대하는 상태는 `mode=2`, `duty=65%`, LED 1개입니다. 예전 모터가 70%를
보내면 LED가 값을 거부하여 AUTO에서 켜진 3개가 그대로 남을 수 있습니다.
기존 모듈이 로드되어 있다면 앱을 종료하고 `sudo rmmod fan_motor led_b led_b_dev`로
내린 뒤 새로 빌드한 LED 장치 → LED 드라이버 → 모터 순서로 다시 로드하세요.

### 미풍에서 LED 3개가 보일 때

먼저 앱과 모터가 같은 모드 번호를 쓰는지 확인해야 합니다. 현재 `fan_motor.h`는
`2=미풍, 3=약풍, 4=중풍, 5=강풍`이지만, `led_motor/fan_motor.zip` 및
`led_motor_bmp/fan_motor.h`에는 `2=약풍, 3=중풍, 4=강풍, 5=미풍`인 구버전이 있습니다.
새 앱의 미풍 명령(2)을 ZIP 안의 구버전 모터·LED에 전달하면 모터는 약풍 70%를
적용하고 LED는 3개를 켭니다. 이 조합은 저장소의 실제 C 함수로 재현했습니다.
앱의 모드 이름 출력은 앱 내부 값이므로, "미풍" 로그만으로 로드된 모터·LED의
버전이 일치한다고 판단할 수 없습니다. 이 통합 폴더에서 앱과 모터·LED를 함께
빌드하고, 기존 모듈을 내린 뒤 같은 폴더의 새 `.ko`를 올려야 합니다.

버튼의 기존 20ms 간격 필터는 누를 때와 뗄 때의 접점 튐을 서로 다른 누름으로
처리할 수 있었습니다. AUTO(1)에서 미풍(2)을 거쳐 약풍(3)으로 한 번 더 넘어가면
LED는 3개가 됩니다. 수정된 `lt_en.c`는 양쪽 신호 변화를 감지하고 20ms 후 안정된
상태를 확인하여, 안정적으로 뗀 뒤 다시 누른 경우에만 다음 이벤트를 만듭니다.
이 수정은 `lt_en.ko`도 다시 빌드하고 교체해야 적용됩니다. 앱을 종료한 뒤
`fnd_driver` → `lt_en` 순서로 내리고, 다시 `lt_en` → `fnd_driver` 순서로 올리세요.
`/dev/lt_en`의 major 번호가 바뀌었으면 `/proc/devices`에 맞춰 노드를 다시 만드세요.

버튼을 한 번 누르고 뗀 뒤 로그를 확인합니다.

```bash
sudo dmesg | grep -E 'KEY|fan_motor:|led_bar:' | tail -40
```

미풍의 정상 로그는 `fan_motor: mode=2 duty=65%`, `led_bar: duty=65 target=1`,
`led_bar: output complete count=1`입니다. 마지막 로그는 소프트웨어 GPIO 출력
완료를 뜻하며 실제 LED 전압을 측정한 결과는 아닙니다.

| 확인 결과 | 다음 확인 대상 |
|---|---|
| 한 번 누르고 뗐는데 mode=2 다음에 mode=3 | 중복 KEY 입력, 교체한 lt_en.ko 확인 |
| mode=1, duty=75 | AUTO 동작: 25.8~26.4°C이면 LED 3개가 정상 |
| duty=70 또는 rejected duty | 다른 버전의 모터/LED 모듈이 로드됨 |
| target=1, output complete count=1인데 실물은 3개 | GPIO 번호, 실제 배선, 핀 설정 확인 |

앱을 계속 실행한 상태에서 현재 모터 상태만 읽으려면 아래 명령을 사용합니다.
이 명령은 모드나 duty를 변경하지 않습니다.

```bash
sudo python3 -c 'import os, struct; fd=os.open("/dev/fan_motor", os.O_RDONLY); print("mode=%d duty=%d%%" % struct.unpack("=II", os.read(fd, 8))); os.close(fd)'
cat /sys/module/led_b_dev/parameters/gpios
cat /sys/module/led_b_dev/parameters/active_low
```

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
