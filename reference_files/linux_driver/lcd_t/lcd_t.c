#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>

#define LCD_RS  0x01    // 0일때 명령어, 1이면 문자
#define LCD_RW  0x02    // 읽기 쓰기
#define LCD_EN  0x04    // 데이터 입력 타이밍 제어어어잇!
#define LCD_BL  0x08    // 뺴꾸라이트

// 디바이스 객체
static struct i2c_client *lcd_client;

static int pcf8574_write(u8 data){
/* i2c 1 byte 전송 */
	int ret;
    // 리눅스 I2C 가져다 쓰기
	ret = i2c_master_send(lcd_client, &data, 1);
	if (ret != 1){
        printk("failed!!!!!!!!!!!!!!\n");
        return -EIO;
    }
	return 0;
}

static void lcd_write4(u8 data){
// 에씨디 펄스 생성하기. 빽라이트는 계속키고, 청기올려 백기내려
	pcf8574_write(data | LCD_BL);
	udelay(1);

	pcf8574_write(data | LCD_BL | LCD_EN);
	udelay(1);

	pcf8574_write(data | LCD_BL);
	udelay(50);
}

static void lcd_send(u8 value, int rs){
// 팔비트 받은 거  4로 쪼개기 
// 지금 4비트 모드로해서 8비트여도 위에 4개만 데이터로 씀
// 아래 4개는 제어 신호호로 해야됨
	u8 high;
	u8 low;

	high = value & 0xF0;
	low  = (value << 4) & 0xF0;

	if (rs) {
		high |= LCD_RS;
		low  |= LCD_RS;
	}

	lcd_write4(high);
	lcd_write4(low);
}


static void lcd_cmd(u8 cmd){
/*
lcd 그 명령어 전송 초기 초기화할때 보내줘야되는거
*/
	lcd_send(cmd, 0);

	if (cmd == 0x01 || cmd == 0x02)
		usleep_range(2000, 2500);
}


static void lcd_data(u8 data){
// 데!이!타! 전송
	lcd_send(data, 1);
}


static void lcd_hw_init(void){
/*
lcd 초기 세팅 그 표있자너
*/
	msleep(50);

	lcd_write4(0x30);
	msleep(5);

	lcd_write4(0x30);
	msleep(5);

	lcd_write4(0x30);
	udelay(200);

	lcd_write4(0x20);

	lcd_cmd(0x28);
	lcd_cmd(0x0C);
	lcd_cmd(0x06);
	lcd_cmd(0x01);
}

// vfs
static ssize_t lcd_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos){

    // 좀 수정해야할 게 있다. 고칠거 개많네 사ㅂㅂㅈㄷㄼㅈㄷ
    // 지금 한줄쓰기 밖에 안됨.
	char kbuf[32];
	size_t len;
	int i;

	len = count;
	if(len > sizeof(kbuf)){
        len = sizeof(kbuf);
    }
	if(copy_from_user(kbuf, buf, len)){
        return -EFAULT;
    }
	for (i = 0; i < len; i++){
        lcd_data(kbuf[i]);
    }
	return len;
}


static const struct file_operations lcd_fops = {
	.owner = THIS_MODULE,
	.write = lcd_write,
};


static struct miscdevice lcd_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "lcd_t",
	.fops  = &lcd_fops,
};

static int lcd_probe(struct i2c_client *client){
/*
초!기!화! i2c 코어에서 호출하는거임 
엔코더랑 다름 문자디바이스를 작성하는 것이아니라
임마가 i2c 드라이버를 쓰는거임
*/ 
	int ret;

	pr_info("lcd_t: probe address = 0x%02x\n", client->addr);

	lcd_client = client;

	lcd_hw_init();

	ret = misc_register(&lcd_miscdev);
	if (ret) {
		pr_err("lct_t: misc_register failed\n");
		return ret;
	}

	pr_info("lcd_t: probe complete\n");

	return 0;
}


static int lcd_remove(struct i2c_client *client){
    misc_deregister(&lcd_miscdev);
    lcd_client = NULL;
    pr_info("lcd_t: removed\n");
    return 0;
}

// i2c 장치 매칭
static const struct i2c_device_id lcd_id[] = {
	{ "lcd_t", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, lcd_id);

// dt만들면 적용될거
static const struct of_device_id lcd_of_match[] = {
	{ .compatible = "mycompany, lcd_t" },
	{ }
};

MODULE_DEVICE_TABLE(of, lcd_of_match);


static struct i2c_driver lcd_driver = {
	.driver = {
		.name = "lcd_t",
		.of_match_table = lcd_of_match,
	},

	.probe_new = lcd_probe,
	.remove = lcd_remove,

	.id_table = lcd_id,
};


// 인있이랑 엑싯트 대신해주는놈
module_i2c_driver(lcd_driver);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("aidl");
MODULE_DESCRIPTION("PCF8574 HD44780 lcd_t I2C Driver");