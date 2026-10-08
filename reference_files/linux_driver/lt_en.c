#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <asm/io.h>

static dev_t devt;              // 주+부 번호 합친 값
static struct cdev *my_cdev;    // 문자 디바이스 객체 주소


#define GPIO_KEY_PHY_BASE	0x02211100  // 0PORT
#define GPIO_S1_PHY_BASE	0x02200500  // 3PORT
#define GPIO_S2_PHY_BASE	0x02211100  // 1PORT
#define GPIO_PHY_SIZE		0x20

#define GPIO_ENABLE_CONFIG	0x00
#define GPIO_INPUT	        0x08
#define GPIO_OUTPUT_CONTROL   0x0C

static volatile unsigned long gpio_key_base;
static volatile unsigned long gpio_s1_base;
static volatile unsigned long gpio_s2_base;

/*
필요한 게 있나?
             Python Jetson.GPIO       libgpiod C
             BOARD 번호                line offset

KEY          24            ───────→     136
S2           26            ───────→     137
S1           33            ───────→      43

일단 디바이스 인있해야됨.

그리고 내가 lcd에 쓸 버퍼가 있어야 겠고
내가 버튼 눌럿을때 값을 인터럽트로 보내줄 게 필요함.
*/

static void gpio_init(void){

    iowrite32((ioread32((void *)(gpio_key_base+GPIO_OUTPUT_CONTROL)) & ~(0x1<<0)) | (0x3<<0),(void *)(gpio_key_base+GPIO_OUTPUT_CONTROL));
    iowrite32((ioread32((void *)(gpio_s1_base+GPIO_OUTPUT_CONTROL)) & ~(0x1<<3)) | (0x3<<0),(void *)(gpio_s1_base+GPIO_OUTPUT_CONTROL));
    iowrite32((ioread32((void *)(gpio_s2_base+GPIO_OUTPUT_CONTROL)) & ~(0x1<<1)) | (0x3<<0),(void *)(gpio_s2_base+GPIO_OUTPUT_CONTROL));
    
	iowrite32((ioread32((void *)(gpio_key_base+GPIO_ENABLE_CONFIG)) & ~(0x1<<0)) | (0x3<<0),(void *)(gpio_key_base+GPIO_ENABLE_CONFIG));
    iowrite32((ioread32((void *)(gpio_s1_base+GPIO_ENABLE_CONFIG)) & ~(0x1<<3)) | (0x3<<0),(void *)(gpio_s1_base+GPIO_ENABLE_CONFIG));
    iowrite32((ioread32((void *)(gpio_s2_base+GPIO_ENABLE_CONFIG)) & ~(0x1<<1)) | (0x3<<0),(void *)(gpio_s2_base+GPIO_ENABLE_CONFIG));

}

static int __init device_init(void){
    int ret;

    gpio_key_base = (unsigned long)ioremap(GPIO_KEY_PHY_BASE,GPIO_PHY_SIZE);
	if (gpio_key_base == 0) {
		printk("devtest: ioremap[gpio_key_base] error\n");
		ret = -EIO;
		goto err3;
	}
    gpio_s1_base = (unsigned long)ioremap(GPIO_S1_PHY_BASE,GPIO_PHY_SIZE);
	if (gpio_s1_base == 0) {
		printk("devtest: ioremap[gpio_s1_base] error\n");
		ret = -EIO;
		goto err3;
	}
    gpio_s2_base = (unsigned long)ioremap(GPIO_S2_PHY_BASE,GPIO_PHY_SIZE);
	if (gpio_s2_base == 0) {
            printk("devtest: ioremap[gpio_s2_base] error\n");
		ret = -EIO;
		goto err3;
	}

    gpio_init();

	return 0;
}



module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");


    iowrite32((ioread32((void *)(gpio_key_base+GPIO_INPUT)) & ~(0x1<<0)),(void *)(gpio_key_base+GPIO_INPUT));
    iowrite32((ioread32((void *)(gpio_s1_base+GPIO_INPUT)) & ~(0x1<<3)),(void *)(gpio_s1_base+GPIO_INPUT));
    iowrite32((ioread32((void *)(gpio_s2_base+GPIO_INPUT)) & ~(0x1<<1)),(void *)(gpio_s2_base+GPIO_INPUT));