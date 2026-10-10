sudo insmod lt_en.ko
lsmod | grep lt_en
dmesg | tail
//sudo dmesg -w
major = 489, minor = 0
sudo mknod /dev/lt_en c 489 0
sudo chmod 666 /dev/lt_en
gcc test.c -o test
./test