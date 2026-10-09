
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/jiffies.h>

//#include "fnd_driver.h"

#define DEVICE_NAME "fnd"
#define FND_GPIO_COUNT 9
#define REFRESH_MS 4

#define FND_DRIVER_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define FND_IOC_MAGIC 'F'

#define FND_IOC_START _IO(FND_IOC_MAGIC, 1)
#define FND_IOC_STOP  _IO(FND_IOC_MAGIC, 2)
#define FND_IOC_RESET _IO(FND_IOC_MAGIC, 3)

/* 남은 초 조회: __u32 * */
#define FND_IOC_GET_REMAINING _IOR(FND_IOC_MAGIC, 4, __u32)

/* read()가 앱에 전달하는 타이머 신호 */
#define FND_TIMER_RUNNING 0
#define FND_TIMER_EXPIRED 1

/* A, B, C, D, E, F, G, DIG3, DIG4 */
static int gpios[FND_GPIO_COUNT] = {
    492, 460, 398, 470, 433, 474, 473, 472, 399
};

static int gpio_count = FND_GPIO_COUNT;
module_param_array(gpios, int, &gpio_count, 0444);

/* 직접 공통 캐소드 연결: DIG LOW 활성 */
static bool digit_active_low = true;
module_param(digit_active_low, bool, 0444);

/* 비트 0~6 = A~G */
static const u8 digit_pattern[10] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66,
    0x6d, 0x7d, 0x07, 0x7f, 0x6f
};

struct fnd_device {
    struct miscdevice misc;
    struct delayed_work refresh_work;
    struct delayed_work timer_work;
    struct mutex lock;

    u32 remaining_sec;
    unsigned long deadline;

    bool running;
    bool expired;
    bool position;
};

static struct platform_device *fnd_pdev;

/* ---------- GPIO 출력 ---------- */

static void fnd_digits_off(void)
{
    gpio_set_value(gpios[7], digit_active_low);
    gpio_set_value(gpios[8], digit_active_low);
}

static void fnd_show_digit(int number, int position)
{
    int i;

    fnd_digits_off();

    for (i = 0; i < 7; i++) {
        gpio_set_value(
            gpios[i],
            !!(digit_pattern[number] & (1U << i))
        );
    }

    gpio_set_value(
        gpios[position == 0 ? 7 : 8],
        !digit_active_low
    );
}

/* ---------- 타이머 내부 함수 ---------- */

/*
 * lock을 획득한 상태에서 호출.
 * 실제 경과 시간을 기준으로 남은 초를 계산.
 */
static void fnd_update_timer(struct fnd_device *fnd)
{
    unsigned long ticks;

    if (!fnd->running)
        return;

    if (time_after_eq(jiffies, fnd->deadline)) {
        fnd->remaining_sec = 0;
        fnd->running = false;
        fnd->expired = true;
        return;
    }

    ticks = fnd->deadline - jiffies;

    fnd->remaining_sec =
        (u32)DIV_ROUND_UP(ticks, HZ);
}

/* ---------- FND 멀티플렉싱 ---------- */

static void fnd_refresh(struct work_struct *work)
{
    struct fnd_device *fnd =
        container_of(
            to_delayed_work(work),
            struct fnd_device,
            refresh_work
        );

    u32 minutes;
    int number;
    int position;

    mutex_lock(&fnd->lock);

    minutes = DIV_ROUND_UP(fnd->remaining_sec, 60);
    position = fnd->position;
    fnd->position = !fnd->position;

    if (position == 0)
        number = minutes / 10;
    else
        number = minutes % 10;

    fnd_show_digit(number, position);

    mutex_unlock(&fnd->lock);

    schedule_delayed_work(
        &fnd->refresh_work,
        msecs_to_jiffies(REFRESH_MS)
    );
}

/* ---------- 1초 단위 카운트다운 ---------- */

static void fnd_timer_tick(struct work_struct *work)
{
    struct fnd_device *fnd =
        container_of(
            to_delayed_work(work),
            struct fnd_device,
            timer_work
        );

    bool just_expired = false;

    mutex_lock(&fnd->lock);

    if (fnd->running) {
        fnd_update_timer(fnd);

        just_expired = fnd->expired;
    }

    mutex_unlock(&fnd->lock);

    if (just_expired)
        pr_info("fnd: timer expired\n");

    /* 1초마다 타이머 상태를 갱신 */
    schedule_delayed_work(
        &fnd->timer_work,
        HZ
    );
}

/* ---------- write(): 시간 설정 ---------- */

static ssize_t fnd_write(
    struct file *file,
    const char __user *buf,
    size_t count,
    loff_t *ppos)
{
    struct fnd_device *fnd =
        container_of(
            file->private_data,
            struct fnd_device,
            misc
        );

    u32 minutes;

    if (count != sizeof(minutes))
        return -EINVAL;

    if (copy_from_user(
            &minutes, buf, sizeof(minutes)))
        return -EFAULT;

    if (minutes > 99)
        return -EINVAL;

    mutex_lock(&fnd->lock);

    fnd->remaining_sec = minutes * 60;
    fnd->running = false;
    fnd->expired = false;

    mutex_unlock(&fnd->lock);

    pr_info("fnd: timer set to %u minutes\n",
            minutes);

    return sizeof(minutes);
}

/* ---------- read(): 완료 상태 ---------- */

static ssize_t fnd_read(
    struct file *file,
    char __user *buf,
    size_t count,
    loff_t *ppos)
{
    struct fnd_device *fnd =
        container_of(
            file->private_data,
            struct fnd_device,
            misc
        );

    u32 timer_signal;

    if (count < sizeof(timer_signal))
        return -EINVAL;

    mutex_lock(&fnd->lock);

    fnd_update_timer(fnd);

    /* 만료되면 1, 아직 시간이 남아 있으면 0을 앱에 전달한다. */
    timer_signal = fnd->expired ?
                   FND_TIMER_EXPIRED : FND_TIMER_RUNNING;

    mutex_unlock(&fnd->lock);

    if (copy_to_user(
            buf, &timer_signal, sizeof(timer_signal)))
        return -EFAULT;

    return sizeof(timer_signal);
}

/* ---------- ioctl(): 시작/정지/초기화 ---------- */

static long fnd_ioctl(
    struct file *file,
    unsigned int cmd,
    unsigned long arg)
{
    struct fnd_device *fnd =
        container_of(
            file->private_data,
            struct fnd_device,
            misc
        );

    u32 seconds;
    long ret = 0;

    mutex_lock(&fnd->lock);

    switch (cmd) {
    case FND_IOC_START:

        if (fnd->running)
            break;

        if (fnd->remaining_sec == 0) {
            ret = -EINVAL;
            break;
        }

        fnd->deadline =
            jiffies + (unsigned long)fnd->remaining_sec * HZ;

        fnd->running = true;
        fnd->expired = false;

        pr_info("fnd: timer start, %u sec\n",
                fnd->remaining_sec);
        break;

    case FND_IOC_STOP:

        fnd_update_timer(fnd);
        fnd->running = false;

        pr_info("fnd: timer stopped, %u sec left\n",
                fnd->remaining_sec);
        break;

    case FND_IOC_RESET:

        fnd->running = false;
        fnd->remaining_sec = 0;
        fnd->expired = false;

        pr_info("fnd: timer reset\n");
        break;

    case FND_IOC_GET_REMAINING:

        fnd_update_timer(fnd);
        seconds = fnd->remaining_sec;

        mutex_unlock(&fnd->lock);

        if (copy_to_user(
                (void __user *)arg,
                &seconds, sizeof(seconds)))
            return -EFAULT;

        return 0;

    default:
        ret = -ENOTTY;
        break;
    }

    mutex_unlock(&fnd->lock);

    return ret;
}

static const struct file_operations fnd_fops = {
    .owner = THIS_MODULE,
    .read = fnd_read,
    .write = fnd_write,
    .unlocked_ioctl = fnd_ioctl,
    .llseek = no_llseek,
};

/* ---------- Platform Driver ---------- */

static int fnd_probe(struct platform_device *pdev)
{
    struct fnd_device *fnd;
    int i, j, ret;

    if (gpio_count != FND_GPIO_COUNT)
        return -EINVAL;

    for (i = 0; i < FND_GPIO_COUNT; i++) {
        if (!gpio_is_valid(gpios[i]))
            return -EINVAL;

        for (j = 0; j < i; j++) {
            if (gpios[j] == gpios[i])
                return -EINVAL;
        }
    }

    fnd = devm_kzalloc(
        &pdev->dev, sizeof(*fnd), GFP_KERNEL
    );

    if (!fnd)
        return -ENOMEM;

    mutex_init(&fnd->lock);

    for (i = 0; i < FND_GPIO_COUNT; i++) {
        int initial = (i < 7) ?
                      0 : digit_active_low;

        ret = devm_gpio_request_one(
            &pdev->dev,
            gpios[i],
            initial ? GPIOF_OUT_INIT_HIGH :
                      GPIOF_OUT_INIT_LOW,
            "fnd_gpio"
        );

        if (ret) {
            dev_err(
                &pdev->dev,
                "GPIO %d request failed: %d\n",
                gpios[i], ret
            );
            return ret;
        }
    }

    fnd->misc.minor = MISC_DYNAMIC_MINOR;
    fnd->misc.name = DEVICE_NAME;
    fnd->misc.fops = &fnd_fops;

    INIT_DELAYED_WORK(
        &fnd->refresh_work, fnd_refresh
    );

    INIT_DELAYED_WORK(
        &fnd->timer_work, fnd_timer_tick
    );

    ret = misc_register(&fnd->misc);
    if (ret)
        return ret;

    platform_set_drvdata(pdev, fnd);

    schedule_delayed_work(
        &fnd->refresh_work, 0
    );

    schedule_delayed_work(
        &fnd->timer_work, HZ
    );

    dev_info(&pdev->dev, "/dev/fnd created\n");

    return 0;
}

static int fnd_remove(struct platform_device *pdev)
{
    struct fnd_device *fnd =
        platform_get_drvdata(pdev);

    misc_deregister(&fnd->misc);

    cancel_delayed_work_sync(
        &fnd->timer_work
    );

    cancel_delayed_work_sync(
        &fnd->refresh_work
    );

    fnd_digits_off();

    return 0;
}

static struct platform_driver fnd_platform_driver = {
    .probe = fnd_probe,
    .remove = fnd_remove,
    .driver = {
        .name = DEVICE_NAME,
    },
};

static int __init fnd_init(void)
{
    int ret;

    ret = platform_driver_register(
        &fnd_platform_driver
    );

    if (ret)
        return ret;

    fnd_pdev = platform_device_register_simple(
        DEVICE_NAME, -1, NULL, 0
    );

    if (IS_ERR(fnd_pdev)) {
        ret = PTR_ERR(fnd_pdev);

        platform_driver_unregister(
            &fnd_platform_driver
        );

        return ret;
    }

    if (!platform_get_drvdata(fnd_pdev)) {
        platform_device_unregister(fnd_pdev);
        platform_driver_unregister(
            &fnd_platform_driver
        );

        return -ENODEV;
    }

    pr_info("fnd: module loaded\n");

    return 0;
}

static void __exit fnd_exit(void)
{
    platform_device_unregister(fnd_pdev);
    platform_driver_unregister(
        &fnd_platform_driver
    );

    pr_info("fnd: module unloaded\n");
}

module_init(fnd_init);
module_exit(fnd_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Jetson Orin Nano FND Timer Driver");
