# LED BAR 핵심 실행 / 제거 (모터 연동)

폴더: `~/work/practice/linux_fan`

## 빌드

```bash
make
```

## 올리기 (이 순서 그대로)

```bash
sudo insmod led_b_dev.ko gpios=458,459,401,400,483,482,481 active_low=0
sudo insmod led_b.ko
sudo insmod <모터모듈>.ko        # 모터 담당자 방식대로 (pwm_id, gpio_in1, gpio_in2 인자)
```

## 확인

```bash
dmesg | tail -5                       # "probe ok" 가 보이면 LED 성공
grep led_set_duty /proc/kallsyms      # 한 줄 나오면 모터가 호출할 수 있는 상태
```

## 제거 (반대 순서)

```bash
sudo rmmod <모터모듈>
sudo rmmod led_b led_b_dev
```

## 꼭 지킬 것

- 올리는 순서: `led_b_dev` → `led_b` → 모터. LED를 모터보다 늦게 올리면 모터가 다음에 duty를 바꿀 때까지 LED가 꺼져 있다.
- 제거 순서: 모터 → `led_b` → `led_b_dev`.
- LED가 반대로 켜지면 `active_low=1` 로 다시 올린다.
- 재부팅하면 모듈이 모두 내려가므로 다시 올려야 한다.

## 모터 duty -> 켜지는 LED

| duty | 단계 | LED |
|---|---|---|
| 0 | OFF | 0개 |
| 100 | 미풍 | 1개 |
| 70 | 약풍 | 3개 |
| 80 | 중풍 | 5개 |
| 90 | 강풍 | 7개 |
