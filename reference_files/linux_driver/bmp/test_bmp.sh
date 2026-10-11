#!/bin/bash
# bmp180 테스트 스크립트
# 사용: sudo ./test_bmp.sh [I2C버스번호(기본 7)]
BUS="${1:-7}"

cd "$(dirname "$0")"
make || exit 1

rmmod bmp180 2>/dev/null

if ! insmod bmp180.ko bus="$BUS"; then
    echo "insmod 실패. 아래 dmesg를 확인하세요."
    dmesg | tail -10
    exit 1
fi

MAJOR=$(dmesg | grep -o 'bmp180: probe ok, major=[0-9]*' | tail -1 | grep -o '[0-9]*$')
if [ -z "$MAJOR" ]; then
    echo "probe 실패. dmesg를 확인하세요."
    dmesg | tail -10
    rmmod bmp180 2>/dev/null
    exit 1
fi

rm -f /dev/bmp180
mknod /dev/bmp180 c "$MAJOR" 0
chmod 666 /dev/bmp180
echo "노드 생성: /dev/bmp180 (major=$MAJOR, i2c bus=$BUS)"

./bmp180_test 5

rmmod bmp180
rm -f /dev/bmp180
echo "완료"
