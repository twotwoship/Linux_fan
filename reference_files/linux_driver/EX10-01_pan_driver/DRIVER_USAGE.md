# Jetson Orin Nano — Fan Motor / FND 드라이버 사용 안내

이 문서는 `fan_motor.c` 및 `fnd_driver.c`의 빌드, 로드, 테스트, 종료 방법을 정리합니다.

> **버전 주의**: 실제 동작과 ioctl/API는 저장소에 올린 `fan_motor.h`, `fnd_driver.h` 및 현재 소스 코드를 기준으로 확인하세요. 아래 설명은 현재 프로젝트의 개발 구성을 기준으로 합니다. 하드웨어 배선과 GPIO 번호는 환경마다 다를 수 있습니다.

## 1. 포함할 소스

```text
fan_motor.c        # 모터 커널 드라이버
fan_motor.h        # 모터 모드(enum), ioctl 정의
fnd_driver.c       # 4자리 FND 타이머 커널 드라이버
fnd_driver.h       # FND ioctl, 회전 알림 API 정의
DRIVER_USAGE.md    # 이 문서
```

**중요:** `.c` 파일만 업로드하면 `#include "fan_motor.h"`, `#include "fnd_driver.h"` 때문에 빌드가 실패할 수 있습니다. 헤더도 함께 커밋하세요. `fnd_notify_rotation()`을 다른 커널 모듈에서 호출할 경우 호출 모듈의 소스 및 빌드 설정은 별도로 필요합니다.

## 2. 하드웨어 관련 설정

- 플랫폼: Jetson Orin Nano, Linux 커널 모듈(`.ko`)
- 모터: L298N 또는 프로젝트에서 실제 사용 중인 구동 회로
- 모터 GPIO: 물리 29(IN1), 31(IN2), PWM 물리 32를 사용했던 구성. Linux GPIO 번호와 PWM 번호는 보드 설정에서 재확인
- 기존 `fan_motor.c` 버전의 로드 파라미터: `pwm_id`, `gpio_in1`, `gpio_in2` (현재 소스에서 실제 지원 여부 확인)
- FND: Common Cathode 4자리 중 DIG3/DIG4 사용, 표시 범위 00~99분
- 이전 FND GPIO 배열(A~G, DIG3, DIG4): `492,460,398,470,433,474,473,472,399`

**전기적 안전:** 세그먼트 A~G에는 각 채널의 전류 제한 저항이 필요합니다. Jetson GPIO의 전류 구동 능력과 FND 자리 선택 회로를 검증하세요. 5V를 Jetson GPIO 입력에 직접 인가하지 마세요. `digit_active_low` 설정은 실제 DIG 구동 회로(직접 공통 캐소드 연결인지, NPN 구동인지)에 맞아야 합니다.

## 3. 빌드

Jetson에서 드라이버 소스와 헤더가 들어 있는 디렉터리로 이동합니다.

```bash
cd ~/work/practice/EX10-01_pan_driver
```

기존 Makefile이 있다면 다음 Kbuild 대상이 들어 있는지 확인합니다.

```makefile
obj-m += fan_motor.o
obj-m += fnd_driver.o
```

`lt_en.ko` 등 다른 모듈까지 함께 빌드하는 기존 Makefile을 사용해도 됩니다. 이때 Git에 올리는 파일 범위는 별도로 선택할 수 있습니다.

```bash
make
ls -lh fan_motor.ko fnd_driver.ko
```

`make`는 현재 Jetson 커널과 일치하는 커널 헤더/빌드 디렉터리가 필요합니다.

## 4. 모듈 로드

모터 드라이버는 **현재 소스에서 모듈 파라미터를 사용하는지 확인한 뒤** 실행합니다. 앞서 사용한 구성의 예:

```bash
sudo insmod ./fan_motor.ko pwm_id=3 gpio_in1=453 gpio_in2=454
```

이미 기본값이 적절히 설정된 최신 소스라면 다음처럼 로드할 수도 있습니다.

```bash
# 파라미터 없이 로드되도록 작성된 버전에서만 사용
sudo insmod ./fan_motor.ko
```

FND 드라이버:

```bash
sudo insmod ./fnd_driver.ko
```

확인:

```bash
lsmod | grep -E 'fan_motor|fnd_driver'
sudo dmesg | tail -30
ls -l /dev/fan_motor /dev/fnd
```

- `File exists`: 이미 같은 이름의 모듈이 로드됨. `lsmod`를 확인한 뒤 필요한 경우 제거/재로드
- `No such file or directory`: `.ko` 파일 경로 또는 의존 파일 확인
- `Unknown symbol`: 다른 모듈의 export 심볼 의존성 및 로드 순서 확인

## 5. 디바이스 인터페이스

### Motor — `/dev/fan_motor`

모터 모드는 다음 enum을 사용합니다(현재 `fan_motor.h`와 대조).

| 값 | 상수 | 의미 |
| ---: | --- | --- |
| 0 | `FAN_OFF` | 정지 |
| 1 | `FAN_AUTO` | 자동 모드 |
| 2 | `FAN_LOW` | 약풍 |
| 3 | `FAN_MEDIUM` | 중풍 |
| 4 | `FAN_HIGH` | 강풍 |
| 5 | `FAN_FINE` | 미풍/추가 설정 모드(실제 Duty는 소스 확인) |

`write(fd, &mode, sizeof(mode))`로 **4바이트 모드 값**을 전달하는 방식입니다. 구체적인 Duty 설정은 커널 드라이버 소스를 기준으로 확인하세요.

기존 소스에 `FAN_IOC_SET_DUTY`, `FAN_IOC_GET_DUTY`가 있었지만 최신 버전에서의 사용 조건은 헤더와 구현을 확인해야 합니다.

### FND — `/dev/fnd`

- `write()` : `uint32_t` 분 단위(0~99) 설정을 지원하는 버전이 있음
- `FND_IOC_START` : 설정 시간 카운트다운 시작
- `FND_IOC_STOP` : 카운트다운 정지
- `FND_IOC_RESET` : 초기화
- `FND_IOC_GET_REMAINING` : 남은 시간(초) 조회
- `read()` : 4바이트 상태 신호. 만료 전 0, 만료 1

현재 프로젝트에서는 엔코더의 CW/CCW를 **사용자 프로그램이 `write()`로 전달하는 대신**, Encoder 커널 드라이버가 export 함수 `fnd_notify_rotation(CW/CCW)`을 호출하는 구조를 목표로 합니다. 이 API 사용 시 `fnd_driver.h`에 enum/프로토타입이 있어야 하고 FND 모듈이 먼저 로드돼야 합니다.

> **구현 상태 주의**: OFF/AUTO에서 FND를 끄는 기능, 회전 처리 지연, 만료 및 RESET 동작은 해당 커밋 버전의 `fnd_driver.c`를 확인하세요. 모든 기능이 구현된 것으로 가정하지 마세요.

## 6. 테스트

`fan_test`, `fnd_test` 실행 파일이 **별도로 존재하고** 현재 드라이버의 인터페이스와 일치하는 경우에만 실행하세요.

```bash
# 예: 모터 모드 2 (LOW), 숫자는 PWM 퍼센트가 아니라 enum 모드
sudo ./fan_test 2

# FND 테스트 프로그램이 있는 경우
sudo ./fnd_test
```

`open: No such file or directory`가 표시되면 소스의 `open()` 경로와 `/dev/fan_motor`, `/dev/fnd` 노드를 점검하세요.

실제 회전 엔코더 연동은 별도 `lt_en.ko`의 GPIO IRQ 동작, 물리 핀 배선, exported symbol 호출을 추가로 검증해야 합니다.

## 7. 모듈 종료

먼저 테스트 응용 프로그램을 종료하세요. Encoder 모듈이 FND export 심볼을 참조한다면 **Encoder를 먼저 제거**해야 합니다.

```bash
# lt_en 모듈이 사용 중인 경우에만
sudo rmmod lt_en
sudo rmmod fnd_driver
sudo rmmod fan_motor
```

`Module is in use`가 나오면 강제 제거하지 말고 열려 있는 프로그램과 의존 모듈을 먼저 확인하세요.

## 8. Git에 관련 파일만 올리기

현재 Git 저장소의 실제 경로에서 아래 명령을 실행합니다.

```bash
git status --short

git add -- fan_motor.c fan_motor.h fnd_driver.c fnd_driver.h DRIVER_USAGE.md

git diff --cached --name-only
# 위 다섯 파일만 스테이징됐는지 확인

git commit -m "Add Jetson fan motor and FND drivers with usage guide"
git push origin main
```

- 이미 다른 파일이 스테이징돼 있으면 `git diff --cached --name-only`에서 모두 표시됩니다. 의도치 않은 파일은 `git restore --staged <파일명>`으로 제외하세요.
- 현재 작업 브랜치가 `main`이 아니면 `git branch --show-current`로 확인하고 실제 브랜치에 push하세요.
- `.ko`, `.o`, `.mod.c`, `Module.symvers`, 실행 파일은 보통 커밋하지 않습니다.
- `Makefile`이 원격 저장소에 없으면 재현 가능한 빌드를 위해 **Makefile도 추가하는 것을 권장**합니다. 단, 사용자 요청대로 파일 범위를 최소화하려면 기존 저장소의 Makefile을 활용하세요.
