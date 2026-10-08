# Linux_fan

# 매우 아주 아주 아주 아주 중요 항상 푸쉬하기전에 풀하기
## Contributors

| 이름 | GitHub | 담당 |
| --- | --- | --- |
| 권민지 | --- | Application, buzzer Driver |
| 김승환 | --- | @@@ Driver |
| 김태환 | --- | LED Driver, Atmospheric Pressure and Temperature Driver |
| 이세형 | --- | Moter Driver, Timer Driver(FND) |
| 이양배 | --- | LCD Driver, Rotary encoder Driver |

## 개발 목표

1. 리눅스 드라이버로 장치를 제어하여 선풍기 구현

## 시스템 구성

```text

  부릉부릉

```

# 0. 범위 / 전제

- 대상: 구현해야 하는 전체
    - 상태머신
    - 각 드라이버
    - 각 디바이스
 

# 1. 시스템 구성 & 하드웨어 리소스 배정

```markdown
  Linux                  
─────────────────                
  @@
```

| 장치 | 신호 | Jetson Orin Nano  핀 | 모듈 물리 핀 |
| --- | --- | --- | --- |
| LCD | signal | 99 | SCL |
| LCD | signal | 98 | SDA |
| piezo buzzer | signal | 33 | + |
| piezo buzzer | GND | 39 | - |
| BMP180| I2C | 3 | SDA |
| BMP180| I2C | 5 | SCL |
| LED BAR| GPIO | 8 | LED1 |
| LED BAR| GPIO | 10 | LED2 |
| LED BAR| GPIO | 35 | LED3 |
| LED BAR| GPIO | 38 | LED4 |
| LED BAR| GPIO | 19 | LED5 |
| LED BAR| GPIO | 21 | LED6 |
| LED BAR| GPIO | 23 | LED7 |
| FND | signal | 7 | GPIO |
| FND | signal | 11 | GPIO |
| FND | signal | 12 | GPIO |
| FND | signal | 13 | GPIO |
| FND | signal | 15 | GPIO |
| FND | signal | 16 | GPIO |
| FND | signal | 18 | GPIO |
| motor | signal | 29 | GPIO |
| motor | signal | 31 | GPIO |
| motor | signal | 32 | PWM |
| FND | signal | 37 | GPIO |
| FND | signal | 40 | GPIO |


## 주요 기능

### Application

- 현재 시각 표시
- 규칙
- 기능


### buzzer Driver

- 현재 시각 표시
- read()
- write()
- ioctl()


### LED Driver

- 현재 시각 표시
- read()
- write()
- ioctl()
- 규칙
- 

### Atmospheric Pressure and Temperature Driver

- 현재 시각 표시
- read()
- write()
- ioctl()
- 규칙


### Moter Driver

- 현재 시각 표시
- read()
- write()
- ioctl()
- 규칙
- 

### Timer Driver(FND)

- 현재 시각 표시
- read()
- write()
- ioctl()
- 규칙
- 

### LCD Driver 

- 현재 시각 표시
- read()
- write()
- ioctl()
- 규칙
- 

### Rotary encoder Driver

- 현재 시각 표시
- read()
- write()
- ioctl()
- 규칙
- 

## Git 및 PR 관리

### Commit 규칙

- 메시지는 `목적:영역: 변경 내용` 형식으로 작성합니다.

```bash
git commit -m "add:lcd: read 함수 추가"
git commit -m "add:fnd: 제어 함수 추가"
git commit -m "rev:app: 로직 수정"
git commit -m "rem:app: 앱 제거"
```
