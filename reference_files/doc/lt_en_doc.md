sudo insmod lt_en.ko
lsmod | grep lt_en
sudo dmesg | tail
//sudo dmesg -w
major = dmesg에 나와있음, minor = 0
sudo mknod /dev/lt_en c 489 0
sudo chmod 666 /dev/lt_en
ls -l /dev/lt_en


gcc test.c -o test
./test

sudo rmmod lt_en.ko
sudo rm /dev/lt_en