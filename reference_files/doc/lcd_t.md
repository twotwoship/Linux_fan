주소 확인  
i2cdetect -l  
bus    = i2c-1
address = 0x27


---------------------------------  
sudo insmod lcd_t.ko  
sudo dmesg | tail  

장치 등록하기  
echo lcd_t 0x27 | sudo tee /sys/bus/i2c/devices/i2c-1/new_device  
sudo dmesg | tail -20  
ls -l /dev/lcd_t  

잡혔는지 확인
sudo i2cdetect -y -r 1  

echo -n "HELLO" | sudo tee /dev/lcd_t  

죽이기
echo 0x27 | sudo tee /sys/bus/i2c/devices/i2c-1/delete_device
sudo rmmod lcd_t

