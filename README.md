# Linux_fan

# 매우 아주 아주 아주 아주 중요 항상 푸쉬하기전에 풀하기

### 환경정보
enviroment.md 참고

## Contributors

| 이름 | GitHub | 담당 |
| --- | --- | --- |
| 권민지 | --- | Application, buzzer Driver |
| 김승환 | --- | Code Maintenance, Build Verification |
| 김태환 | --- | LED Driver, Atmospheric Pressure and Temperature Driver |
| 이세형 | --- | Moter Driver, Timer Driver(FND) |
| 이양배 | --- | LCD Driver, Rotary encoder Driver |

## 개발 목표

1. 리눅스 드라이버로 장치를 제어하여 선풍기 구현

## 시스템 구성

```text
Rotary Encoder KEY ──> lt_en Driver ──> Application ──> Motor Driver ──> DC Motor
                               │              │                │
                               │              ├──> FND Driver  └──> LED Driver ──> LED BAR
                               │              └──> Buzzer Driver ──> Piezo Buzzer
                               └─ CW/CCW ────> FND Driver ──> FND

BMP180 ──> BMP180 Driver ──┬──> Motor Driver(AUTO)
                           └──> LCD Driver ──> LCD
```

# 0. 범위 / 전제

- 대상: 구현해야 하는 전체
    - 상태머신
    - 각 드라이버
    - 각 디바이스
 

# 1. 시스템 구성 & 하드웨어 리소스 배정

```markdown
User Space
────────────────────────────────────────────────────────────
  fan_app
    ├─ /dev/lt_en       : KEY 입력
    ├─ /dev/fan_motor   : 팬 모드 제어
    ├─ /dev/fnd         : 타이머 상태 및 표시 제어
    └─ /dev/buzz_t      : 타이머 만료 알림

Linux Kernel
────────────────────────────────────────────────────────────
  lt_en ── CW/CCW FIFO ──> fnd_driver
  bmp180 ── 최신 온도/기압 ──┬─> fan_motor ──> led_b
                           └─> lcd_t

Hardware
────────────────────────────────────────────────────────────
  Rotary Encoder | FND | BMP180 | DC Motor | LED BAR | LCD | Piezo Buzzer
```

| 장치 | 신호 | Jetson Orin Nano  핀 | 모듈 물리 핀 |
| --- | --- | --- | --- |
| LCD | I2C | 27 | SDA |
| LCD | I2C | 28 | SCL |
| Rotary Enconder | GPIO | 24 | KEY |
| Rotary Enconder | GPIO | 26 | S2 |
| Rotary Enconder | GPIO | 22 | S1 |
| piezo buzzer | PWM | 33 | + |
| BMP180| I2C | 3 | SDA |
| BMP180| I2C | 5 | SCL |
| LED BAR| GPIO | 8 | LED1 |
| LED BAR| GPIO | 10 | LED2 |
| LED BAR| GPIO | 35 | LED3 |
| LED BAR| GPIO | 38 | LED4 |
| LED BAR| GPIO | 19 | LED5 |
| LED BAR| GPIO | 21 | LED6 |
| LED BAR| GPIO | 23 | LED7 |
| FND | GPIO | 7 | FND A |
| FND | GPIO | 11 | FND B|
| FND | GPIO | 12 | FND C |
| FND | GPIO | 13 | FND D |
| FND | GPIO | 15 | FND E |
| FND | GPIO | 16 | FND F|
| FND | GPIO | 18 | FND G |
| motor | GPIO | 29 | IN1 |
| motor | GPIO | 31 | IN2 |
| motor | PWM | 32 | EnableA |
| FND | GPIO | 37 | DIG3 |
| FND | GPIO | 40 | DIG4 |


## 주요 기능

### Application

- 로터리 엔코더의 KEY 입력으로 팬 모드를 전환하고, 수동 모드에서는 FND 타이머를 시작·감시합니다.
- 타이머가 만료되면 모터를 끄고 FND를 초기화한 뒤 부저 알림음을 재생합니다.


### buzzer Driver

- PWM으로 정해진 알림음을 재생하며, `/dev/buzz_t`에 정수 `1`을 쓰면 재생 작업을 요청합니다.
- `read()`는 지원하지 않으며 애플리케이션의 타이머 만료 알림에 사용됩니다.


### LED Driver

- 모터의 PWM 듀티에 따라 7개 LED의 점등 개수를 정하고, 목표 개수까지 한 칸씩 점등하거나 소등합니다.
- 커널 함수와 `ioctl()`로 듀티 값을 받아 팬 세기를 LED BAR에 표시합니다.

### Atmospheric Pressure and Temperature Driver

- BMP180의 보정 계수를 읽고 I2C로 온도와 기압을 측정해 정수 단위의 보정값을 제공합니다.
- `ioctl()` 측정과 1초 주기의 최신값 저장을 지원하여 모터 자동 제어와 LCD 표시에서 사용합니다.


### Moter Driver

- 팬 모드에 따라 GPIO 방향과 PWM 듀티를 제어하며, AUTO 모드에서는 BMP180 온도에 맞춰 풍량을 조절합니다.
- `write()`로 모드를 설정하고 `read()`와 `ioctl()`로 현재 모드·듀티를 조회하거나 AUTO 듀티를 설정합니다.

### Timer Driver(FND)

- 두 자리 FND에 0~99분의 남은 시간을 표시하고 로터리 회전에 따라 1분씩 증감하며 카운트다운합니다.
- `read()`로 만료 상태를 전달하고 `ioctl()`로 시작·정지·초기화·남은 시간 조회와 표시 ON/OFF를 처리합니다.

### LCD Driver 

- PCF8574를 통해 HD44780 LCD를 I2C로 제어하고, BMP180의 최신 온도와 기압을 1초마다 두 줄에 표시합니다.
- `/dev/lcd_t`의 `write()`로 전달된 문자도 LCD에 출력합니다.

### Rotary encoder Driver

- KEY 입력을 디바운싱해 `read()`와 `poll()`로 애플리케이션에 전달하며 `write()`는 지원하지 않습니다.
- S1·S2 회전 인터럽트로 CW/CCW를 판별해 FIFO에 저장하고 FND 타이머가 이를 읽도록 제공합니다.

## Git 및 PR 관리

### Commit 규칙

- 메시지는 `목적:영역: 변경 내용` 형식으로 작성합니다.

```bash
git commit -m "add:lcd: read 함수 추가"
git commit -m "add:fnd: 제어 함수 추가"
git commit -m "rev:app: 로직 수정"
git commit -m "rem:app: 앱 제거"
```
