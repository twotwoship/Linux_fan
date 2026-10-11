# Development Environment

## Hardware

| Item | Information |
|---|---|
| Board | NVIDIA Jetson Orin Nano Engineering Reference Developer Kit Super |
| Architecture | AArch64 |
| SoC Platform | NVIDIA Jetson Orin / Tegra234 family |

---

## Operating System

| Item | Version |
|---|---|
| OS | Ubuntu 22.04.5 LTS |
| Codename | Jammy Jellyfish |
| Architecture | aarch64 |

---

## Linux Kernel

| Item | Information |
|---|---|
| Kernel Version | `5.15.199-tegra` |
| Kernel Type | NVIDIA Tegra Kernel |
| Preemption | PREEMPT |
| Build Date | 2026-07-16 |

```text
Linux ubuntu 5.15.199-tegra #1 SMP PREEMPT
```

---

## NVIDIA Linux for Tegra (L4T)

| Item | Information |
|---|---|
| L4T Release | R36.5.2 |
| L4T Core Package | `36.5.2-20260716114719` |
| EABI | aarch64 |
| Kernel Variant | oot |

```text
# R36 (release), REVISION: 5.2
# KERNEL_VARIANT: oot
```

> `nvidia-jetpack` 메타 패키지는 현재 시스템에서 확인되지 않았습니다.  
> 따라서 개발 환경 버전은 JetPack 버전 대신 L4T `R36.5.2`를 기준으로 기록합니다.

---

## Compiler

| Item | Version |
|---|---|
| GCC | `11.4.0` |
| GCC Package | `Ubuntu 11.4.0-1ubuntu1~22.04.3` |
| Target Architecture | aarch64 |

---

## Kernel Headers

Kernel module build에 사용하는 Linux kernel header/source 경로입니다.

```text
/usr/src/linux-headers-5.15.199-tegra-ubuntu22.04_aarch64/3rdparty/canonical/linux-jammy/kernel-source
```

일반적인 kernel build 링크는 다음 경로를 사용합니다.

```bash
/lib/modules/$(uname -r)/build
```

현재 시스템에서는 위 링크가 다음 경로를 가리킵니다.

```text
/usr/src/linux-headers-5.15.199-tegra-ubuntu22.04_aarch64/3rdparty/canonical/linux-jammy/kernel-source
```

---

## Environment Summary

```text
Board        : NVIDIA Jetson Orin Nano Engineering Reference Developer Kit Super
OS           : Ubuntu 22.04.5 LTS (Jammy Jellyfish)
Architecture : aarch64
Kernel       : 5.15.199-tegra
L4T          : R36.5.2
L4T Core     : 36.5.2-20260716114719
Kernel Type  : NVIDIA Tegra
GCC          : 11.4.0
JetPack      : Not detected
```

---

## Driver Development Notes

이 프로젝트의 Linux device driver는 현재 환경을 기준으로 개발 및 테스트했습니다.

특히 다음 항목은 커널 모듈 빌드 및 동작에 영향을 줄 수 있습니다.

- Linux Kernel: `5.15.199-tegra`
- NVIDIA L4T: `R36.5.2`
- Architecture: `aarch64`
- GCC: `11.4.0`
- Ubuntu: `22.04.5 LTS`

다른 Jetson 또는 다른 L4T / Kernel 버전에서 사용할 경우 GPIO, Device Tree, kernel API 및 kernel header 구성이 달라질 수 있으므로 확인이 필요합니다.
