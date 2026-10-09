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
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>


static dev_t devt;              // 주+부 번호 합친 값
static struct cdev *lt_en_cdev;    // 문자 디바이스 객체 주소
static int device_in_use = 0;
static unsigned int device_minor_start = 0;
static unsigned int device_minor_count = 1;

#define GPIO_KEY_PHY_BASE	0x02211100  // 0PORT 
#define GPIO_S1_PHY_BASE	0x02200500  // 3PORT
#define GPIO_S2_PHY_BASE	0x02211100  // 1PORT
#define GPIO_PHY_SIZE		0x20

#define GPIO_ENABLE_CONFIG	0x00
#define GPIO_INPUT	        0x08        // input value
#define GPIO_OUTPUT_CONTROL   0x0C

static volatile unsigned long gpio_key_base;
static volatile unsigned long gpio_s1_base;
static volatile unsigned long gpio_s2_base;

// legacy GPIO 번호 = 348 + line offset
#define GPIO_KEY  484   //136	PZ.06
#define GPIO_S1   391   //43	PH.00
#define GPIO_S2   485   //137	PZ.07

/* 디바운싱용 */
#define DEBOUNCE_TIME_MS 20
static unsigned long last_interrupt_time;

/* 사용자에서 key 인터럽트를 위한 큐*/
static DECLARE_WAIT_QUEUE_HEAD(key_wait_queue);
static atomic_t key_event_count = ATOMIC_INIT(0);

static int irq_key;
static int irq_s1;
static int irq_s2;


static int old_s1;
static int old_s2;


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
    // 1. KEY (Port 17, Bit 0 - Line offset 136)
    iowrite32((ioread32((void *)(gpio_key_base + GPIO_OUTPUT_CONTROL)) & ~(0x1 << 0)), (void *)(gpio_key_base + GPIO_OUTPUT_CONTROL));
    iowrite32((ioread32((void *)(gpio_key_base + GPIO_ENABLE_CONFIG)) | (0x1 << 0)), (void *)(gpio_key_base + GPIO_ENABLE_CONFIG));

    // 2. S2 (Port 17, Bit 1 - Line offset 137) *gpio_key_base와 동일 포트
    iowrite32((ioread32((void *)(gpio_s2_base + GPIO_OUTPUT_CONTROL)) & ~(0x1 << 1)), (void *)(gpio_s2_base + GPIO_OUTPUT_CONTROL));
    iowrite32((ioread32((void *)(gpio_s2_base + GPIO_ENABLE_CONFIG)) | (0x1 << 1)), (void *)(gpio_s2_base + GPIO_ENABLE_CONFIG));

    // 3. S1 (Port 5, Bit 3 - Line offset 43)
    iowrite32((ioread32((void *)(gpio_s1_base + GPIO_OUTPUT_CONTROL)) & ~(0x1 << 3)), (void *)(gpio_s1_base + GPIO_OUTPUT_CONTROL));
    iowrite32((ioread32((void *)(gpio_s1_base + GPIO_ENABLE_CONFIG)) | (0x1 << 3)), (void *)(gpio_s1_base + GPIO_ENABLE_CONFIG));
}

static int lt_en_open(struct inode *inode, struct file *file)
{	
    printk("devtest: lt_en_open (minor = %d)\n", iminor(inode));
	if (device_in_use) {
		return -EBUSY;
	}
	device_in_use++;
	try_module_get(THIS_MODULE);
	return 0;
}

static int lt_en_release(struct inode *inode, struct file *file)
{
	printk("devtest: lt_en_release\n");
    device_in_use--;
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t lt_en_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
    return -EOPNOTSUPP;
}

static ssize_t lt_en_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
    int ret;
    int value = 1;

    if (count < sizeof(value))
    return -EINVAL;

    ret = wait_event_interruptible(key_wait_queue, atomic_read(&key_event_count) > 0);
    if(ret) return ret;

    if(copy_to_user(buf,&value, sizeof(value))){
        return - EFAULT;
    }
    atomic_dec(&key_event_count);

    return sizeof(value);
}

static __poll_t lt_en_poll(struct file *filp, poll_table *wait)
{
    __poll_t mask = 0;

    poll_wait(filp, &key_wait_queue, wait);

    if (atomic_read(&key_event_count) > 0){
        mask |= POLLIN | POLLRDNORM;
    }
    return mask;
}

static const struct file_operations lt_en_fops = {
    .owner = THIS_MODULE,
    .open = lt_en_open,
    .release = lt_en_release,
    .read = lt_en_read,
    .write = lt_en_write,
    .poll = lt_en_poll,
    //.unlocked_ioctl = 
};

static irqreturn_t key_isr(int irq, void *dev_id){
    unsigned long current_time = jiffies;
    if(time_after(current_time, last_interrupt_time + msecs_to_jiffies(DEBOUNCE_TIME_MS)){
        atomic_inc(&key_event_count);
        last_interrupt_time = current_time;
        wake_up_interruptible(&key_wait_queue);
    }
    return IRQ_HANDLED;
}

static irqreturn_t s_isr(int irq, void *dev_id){
    /*
    INIT 00 / S1 S2
     CW    01    11    10    00
    CCW    10    11    01    00
    */

    
    return IRQ_HANDLED;
}

static int __init lt_en_init(void){
    int ret;

    printk("lt_en _init\n");

    ret = alloc_chrdev_region(&devt, 0, 1, "lt_en");
    if(ret < 0) {
    printk("lt_en: alloc_chrdev_region failed taaaaaaaaaaaie!!!!!!\n");
		goto err0;
	}

    printk("major = %u, minor = %u \n", MAJOR(devt), MINOR(devt));

    lt_en_cdev = cdev_alloc();
    lt_en_cdev->ops = &lt_en_fops;
    lt_en_cdev->owner = THIS_MODULE;
    ret = cdev_add(lt_en_cdev, devt, device_minor_count);
    if(ret){
        printk("cdev_add fail %d\n", devt);
        goto err1;
    }

    gpio_key_base = (unsigned long)ioremap(GPIO_KEY_PHY_BASE,GPIO_PHY_SIZE);
	if (gpio_key_base == 0) {
		printk("devtest: ioremap[gpio_key_base] error\n");
		ret = -EIO;
		goto err2;
	}
    gpio_s1_base = (unsigned long)ioremap(GPIO_S1_PHY_BASE,GPIO_PHY_SIZE);
	if (gpio_s1_base == 0) {
		printk("devtest: ioremap[gpio_s1_base] error\n");
		ret = -EIO;
		goto err3;
	}
    gpio_s2_base = gpio_key_base;

    gpio_init();

    irq_key = gpio_to_irq(GPIO_KEY);
    if(request_irq(irq_key, key_isr, IRQF_TRIGGER_FALLING, "key_irq", NULL)){
        printk("lt_en : IRQ %d noooooooooooooooooo\n", irq_key);
        ret = -EIO;
        goto err4;
    }
    irq_s1 = gpio_to_irq(GPIO_S1);
    if(request_irq(irq_s1, s_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "s_irq", NULL)){
        printk("lt_en : IRQ %d noooooooooooooooooo\n", irq_s1);
        ret = -EIO;
        goto err4;
    }
    irq_s2 = gpio_to_irq(GPIO_S2);
    if(request_irq(irq_s2, s_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "s_irq", NULL)){
        printk("lt_en : IRQ %d noooooooooooooooooo\n", irq_s2);
        ret = -EIO;
        goto err4;
    }

    printk("KEY GPIO=%d IRQ=%d\n", GPIO_KEY, irq_key);
    printk("S2  GPIO=%d IRQ=%d\n", GPIO_S2, irq_s2);
    printk("S1  GPIO=%d IRQ=%d\n", GPIO_S1, irq_s1);

    return 0;


err4:
    iounmap((void *)gpio_key_base);
    iounmap((void *)gpio_s1_base);
err3:
err2:
err1:
    cdev_del(lt_en_cdev);
    unregister_chrdev_region(devt, device_minor_count);
err0:
    return ret;
}

static void __exit lt_en_exit(void)
{
    printk("si-ma-ei : lt_en device exit \n");

    free_irq(irq_key,NULL);
    free_irq(irq_s1,NULL);
    free_irq(irq_s2,NULL);

    iounmap(gpio_s1_base);
    iounmap(gpio_key_base);
    cdev_del(lt_en_cdev);
    unregister_chrdev_region(devt, 1);
}

module_init(lt_en_init);
module_exit(lt_en_exit);

MODULE_LICENSE("GPL");

