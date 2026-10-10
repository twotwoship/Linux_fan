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

#define GPIO_KEY_PHY_BASE  0x022114C0  // PZ.06, BOARD 24
#define GPIO_S1_PHY_BASE   0x02214200  // PH.00, BOARD 33
#define GPIO_S2_PHY_BASE   0x022114E0  // PZ.07, BOARD 26
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

// 인터럽트용
static int irq_key;
static int irq_s1;
static int irq_s2;

// cw ccw 구분용
static unsigned char prev_s = 0;
static int rotary_count = 0;

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
    unsigned int value;
    int s1;
    int s2;

    /* KEY : PZ.06 input */
    value = ioread32((void *)(gpio_key_base + GPIO_OUTPUT_CONTROL));
    value |= (0x1 << 0);
    iowrite32(value, (void *)(gpio_key_base + GPIO_OUTPUT_CONTROL));

    value = ioread32((void *)(gpio_key_base + GPIO_ENABLE_CONFIG));
    value |= (0x1 << 0);      // GPIO enable
    value &= ~(0x1 << 1);     // input
    iowrite32(value, (void *)(gpio_key_base + GPIO_ENABLE_CONFIG));

    /* S1 : PH.00 input */
    value = ioread32((void *)(gpio_s1_base + GPIO_OUTPUT_CONTROL));
    value |= (0x1 << 0);
    iowrite32(value, (void *)(gpio_s1_base + GPIO_OUTPUT_CONTROL));

    value = ioread32((void *)(gpio_s1_base + GPIO_ENABLE_CONFIG));
    value |= (0x1 << 0);
    value &= ~(0x1 << 1);
    iowrite32(value, (void *)(gpio_s1_base + GPIO_ENABLE_CONFIG));

    /* S2 : PZ.07 input */
    value = ioread32((void *)(gpio_s2_base + GPIO_OUTPUT_CONTROL));
    value |= (0x1 << 0);
    iowrite32(value, (void *)(gpio_s2_base + GPIO_OUTPUT_CONTROL));

    value = ioread32((void *)(gpio_s2_base + GPIO_ENABLE_CONFIG));
    value |= (0x1 << 0);
    value &= ~(0x1 << 1);
    iowrite32(value, (void *)(gpio_s2_base + GPIO_ENABLE_CONFIG));

    s1 = ioread32((void *)(gpio_s1_base + GPIO_INPUT)) & 0x1;
    s2 = ioread32((void *)(gpio_s2_base + GPIO_INPUT)) & 0x1;

    prev_s = (s1 << 1) | s2;

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

irqreturn_t key_isr(int irq, void *dev_id){
    unsigned long current_time = jiffies;
    if(time_after(current_time, last_interrupt_time + msecs_to_jiffies(DEBOUNCE_TIME_MS))){
        atomic_inc(&key_event_count);
        last_interrupt_time = current_time;
        wake_up_interruptible(&key_wait_queue);
        printk("KEY\n");
    }
    return IRQ_HANDLED;
}

static const int rotary_table[16] = {
    /*
    cw  0001 1  0111 7  1110 14 1000 8
    ccw 0010 2  1011 11 1101 13 0100 4
    */
     0,  1, -1,  0,
    -1,  0,  0,  1,
     1,  0,  0, -1,
     0, -1,  1,  0
};

static irqreturn_t s_isr(int irq, void *dev_id){
    int s1;
    int s2;
    unsigned char curr_s;
    unsigned char tran_s;

    /*
    INIT 00 / S1 S2
     CW    01    11    10    00
    CCW    10    11    01    00
    */
    
    s1 = ioread32((void *)(gpio_s1_base + GPIO_INPUT)) & 0x1;
    s2 = ioread32((void *)(gpio_s2_base + GPIO_INPUT)) & 0x1;
    
    curr_s = (s1 << 1) | (s2);
    tran_s = (prev_s << 2) | curr_s;
    rotary_count += rotary_table[tran_s];
    prev_s = curr_s;
    
    if(rotary_count >= 4){
        /* cw */
        printk("CW\n");
        rotary_count = 0;
    }else if(rotary_count <= -4){
        /* ccw */
         printk("CCW\n");
        rotary_count = 0;
    }

    return IRQ_HANDLED;
}

static int __init lt_en_init(void){
    int ret;

    printk("lt_en _init\n");

    ret = alloc_chrdev_region(&devt, device_minor_start, device_minor_count, "lt_en");
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
    gpio_s2_base = (unsigned long)ioremap(GPIO_S2_PHY_BASE, GPIO_PHY_SIZE);
    if (gpio_s2_base == 0) {
        printk("lt_en: ioremap[gpio_s2_base] error\n");
        ret = -EIO;
        goto err4;
    }

    gpio_init();

    irq_key = gpio_to_irq(GPIO_KEY);
    if(request_irq(irq_key, key_isr, IRQF_TRIGGER_FALLING, "key_irq", NULL)){
        printk("lt_en : IRQ %d noooooooooooooooooo\n", irq_key);
        ret = -EIO;
        goto err5;
    }
    irq_s1 = gpio_to_irq(GPIO_S1);
    if(request_irq(irq_s1, s_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "s_irq", NULL)){
        printk("lt_en : IRQ %d noooooooooooooooooo\n", irq_s1);
        ret = -EIO;
        goto err6;
    }
    irq_s2 = gpio_to_irq(GPIO_S2);
    if(request_irq(irq_s2, s_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "s_irq", NULL)){
        printk("lt_en : IRQ %d noooooooooooooooooo\n", irq_s2);
        ret = -EIO;
        goto err7;
    }

    printk("KEY GPIO=%d IRQ=%d\n", GPIO_KEY, irq_key);
    printk("S2  GPIO=%d IRQ=%d\n", GPIO_S2, irq_s2);
    printk("S1  GPIO=%d IRQ=%d\n", GPIO_S1, irq_s1);

    return 0;

err7:
    free_irq(irq_s1, NULL);
err6:
    free_irq(irq_key, NULL);
err5:
    iounmap((void *)gpio_s2_base);
err4:
    iounmap((void *)gpio_s1_base);
err3:
    iounmap((void *)gpio_key_base);
err2:
    cdev_del(lt_en_cdev);
err1:
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
    iounmap(gpio_s2_base);
    iounmap(gpio_key_base);
    
    cdev_del(lt_en_cdev);
    unregister_chrdev_region(devt, 1);
}

module_init(lt_en_init);
module_exit(lt_en_exit);

MODULE_LICENSE("GPL");

