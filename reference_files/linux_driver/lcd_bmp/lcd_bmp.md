비루두 순서

sudo insmod bmp180.ko
sudo insmod lcd_t.ko

echo lcd_t 0x27 | sudo tee /sys/bus/i2c/devices/i2c-1/new_device  


내리는 순서

echo 0x27 | sudo tee /sys/bus/i2c/devices/i2c-1/delete_device
sudo rmmod lcd_t
sudo rmmod bmp180