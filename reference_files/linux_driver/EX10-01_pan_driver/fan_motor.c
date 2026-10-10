
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "fan_motor.h"

#define DEVICE_NAME   "fan_motor"
#define CLASS_NAME    "fan"
#define PWM_PERIOD_NS 50000ULL
#define PWM_ID3 3
#define GPIO_IN1 453
#define GPIO_IN2 454

/*
 * 모듈 로드 시 전달하는 번호
 *
 * pwm_id  : PWM7 전역 ID
 * gpio_in1: 물리 29번 핀의 Linux GPIO 번호
 * gpio_in2: 물리 31번 핀의 Linux GPIO 번호
 */
static int pwm_id = PWM_ID3;
static int gpio_in1 = GPIO_IN1;
static int gpio_in2 = GPIO_IN2;

module_param(pwm_id, int, 0444);
MODULE_PARM_DESC(pwm_id, "Global PWM ID for motor ENA");

module_param(gpio_in1, int, 0444);
MODULE_PARM_DESC(gpio_in1, "Linux GPIO number for L298N IN1");

module_param(gpio_in2, int, 0444);
MODULE_PARM_DESC(gpio_in2, "Linux GPIO number for L298N IN2");

/* ---------------------------------------
 * Fan Device
 * --------------------------------------- */
struct fan_device {
    struct pwm_device *pwm;

    dev_t devno;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    struct mutex lock;

    __u32 mode;
    __u32 duty;
};

/* ---------------------------------------
 * Motor Direction
 *
 * IN1 = HIGH
 * IN2 = LOW
 * --------------------------------------- */
static void fan_direction_forward(void)
{
    gpio_set_value(gpio_in1, 1);
    gpio_set_value(gpio_in2, 0);
}

/* ---------------------------------------
 * Motor Stop
 *
 * IN1 = LOW
 * IN2 = LOW
 * --------------------------------------- */
static void fan_direction_stop(void)
{
    gpio_set_value(gpio_in1, 0);
    gpio_set_value(gpio_in2, 0);
}

/* ---------------------------------------
 * PWM Duty 설정
 * --------------------------------------- */
static int fan_apply_duty(struct fan_device *fan,
                          __u32 duty)
{
    struct pwm_state state;
    int ret;

    if (duty > 100)
        return -EINVAL;

    pwm_get_state(fan->pwm, &state);

    state.period = PWM_PERIOD_NS;
    state.duty_cycle =
        PWM_PERIOD_NS * duty / 100;

    state.polarity = PWM_POLARITY_NORMAL;
    state.enabled = (duty > 0);

    /*
     * PWM 활성화 전에 방향 설정
     */
    if (duty > 0)
        fan_direction_forward();

    ret = pwm_apply_state(fan->pwm, &state);
    if (ret)
        return ret;

    /*
     * Duty 0이면 방향 GPIO도 LOW
     */
    if (duty == 0)
        fan_direction_stop();

    fan->duty = duty;

    pr_info("fan_motor: duty=%u%%\n", duty);

    return 0;
}

/* ---------------------------------------
 * Motor Mode 설정
 * --------------------------------------- */
static int fan_set_mode(struct fan_device *fan,
                        __u32 mode)
{
    __u32 duty;
    int ret;

    switch (mode) {
    case FAN_OFF:
    case FAN_AUTO:
        duty = 0;
        break;

    case FAN_LOW:
        duty = 70;
        break;

    case FAN_MEDIUM:
        duty = 80;
        break;

    case FAN_HIGH:
        duty = 90;
        break;

    case FAN_FINE:
        duty = 100;
        break;

    default:
        return -EINVAL;
    }

    ret = fan_apply_duty(fan, duty);
    if (ret)
        return ret;

    fan->mode = mode;

    return 0;
}

/* ---------------------------------------
 * open()
 * --------------------------------------- */
static int fan_open(struct inode *inode,
                    struct file *file)
{
    struct fan_device *fan;

    fan = container_of(inode->i_cdev,
                       struct fan_device,
                       cdev);

    file->private_data = fan;

    return 0;
}

/* ---------------------------------------
 * write()
 *
 * __u32 mode 전달
 * --------------------------------------- */
static ssize_t fan_write(struct file *file,
                         const char __user *buf,
                         size_t count,
                         loff_t *ppos)
{
    struct fan_device *fan = file->private_data;
    __u32 mode;
    int ret;

    if (count != sizeof(mode))
        return -EINVAL;

    if (copy_from_user(&mode, buf, sizeof(mode)))
        return -EFAULT;

    mutex_lock(&fan->lock);

    ret = fan_set_mode(fan, mode);

    mutex_unlock(&fan->lock);

    if (ret)
        return ret;

    return sizeof(mode);
}

/* ---------------------------------------
 * read()
 *
 * mode + duty 반환
 * --------------------------------------- */
static ssize_t fan_read(struct file *file,
                        char __user *buf,
                        size_t count,
                        loff_t *ppos)
{
    struct fan_device *fan = file->private_data;
    struct fan_status status;

    if (count < sizeof(status))
        return -EINVAL;

    mutex_lock(&fan->lock);

    status.mode = fan->mode;
    status.duty = fan->duty;

    mutex_unlock(&fan->lock);

    if (copy_to_user(buf, &status,
                     sizeof(status)))
        return -EFAULT;

    return sizeof(status);
}

/* ---------------------------------------
 * ioctl()
 * --------------------------------------- */
static long fan_ioctl(struct file *file,
                      unsigned int cmd,
                      unsigned long arg)
{
    struct fan_device *fan = file->private_data;
    __u32 duty;
    int ret = 0;

    switch (cmd) {

    case FAN_IOC_SET_DUTY:

        if (copy_from_user(&duty,
                           (void __user *)arg,
                           sizeof(duty)))
            return -EFAULT;

        mutex_lock(&fan->lock);

        if (fan->mode != FAN_AUTO)
            ret = -EINVAL;
        else
            ret = fan_apply_duty(fan, duty);

        mutex_unlock(&fan->lock);

        return ret;

    case FAN_IOC_GET_DUTY:

        mutex_lock(&fan->lock);
        duty = fan->duty;
        mutex_unlock(&fan->lock);

        if (copy_to_user((void __user *)arg,
                         &duty,
                         sizeof(duty)))
            return -EFAULT;

        return 0;

    default:
        return -ENOTTY;
    }
}

/* ---------------------------------------
 * File Operations
 * --------------------------------------- */
static const struct file_operations fan_fops = {
    .owner          = THIS_MODULE,
    .open           = fan_open,
    .read           = fan_read,
    .write          = fan_write,
    .unlocked_ioctl = fan_ioctl,
    .llseek         = no_llseek,
};

/* ---------------------------------------
 * probe()
 * --------------------------------------- */
static int fan_probe(struct platform_device *pdev)
{
    struct fan_device *fan;
    int ret;

    dev_info(&pdev->dev, "fan probe start\n");

    if (pwm_id < 0 ||
        !gpio_is_valid(gpio_in1) ||
        !gpio_is_valid(gpio_in2) ||
        gpio_in1 == gpio_in2) {

        dev_err(&pdev->dev,
                "Invalid PWM/GPIO parameters\n");
        return -EINVAL;
    }

    fan = devm_kzalloc(&pdev->dev,
                       sizeof(*fan),
                       GFP_KERNEL);
    if (!fan)
        return -ENOMEM;

    mutex_init(&fan->lock);

    /*
     * IN1 GPIO 요청
     */
    ret = devm_gpio_request_one(
        &pdev->dev,
        gpio_in1,
        GPIOF_OUT_INIT_LOW,
        "fan_in1"
    );
    if (ret)
        return ret;

    /*
     * IN2 GPIO 요청
     */
    ret = devm_gpio_request_one(
        &pdev->dev,
        gpio_in2,
        GPIOF_OUT_INIT_LOW,
        "fan_in2"
    );
    if (ret)
        return ret;

    /*
     * PWM 요청
     */
    fan->pwm = pwm_request(pwm_id, DEVICE_NAME);

    if (IS_ERR(fan->pwm)) {
        ret = PTR_ERR(fan->pwm);

        dev_err(&pdev->dev,
                "PWM request failed: %d\n", ret);

        return ret;
    }
    if (strcmp(dev_name(fan->pwm->chip->dev),
               "32e0000.pwm") != 0) {

        dev_err(&pdev->dev,
                "Wrong PWM controller: %s\n",
                dev_name(fan->pwm->chip->dev));

        ret = -ENODEV;
        goto err_pwm;
    }

    fan->mode = FAN_OFF;
    fan->duty = 0;

    /*
     * 초기 상태: 모터 정지
     */
    ret = fan_apply_duty(fan, 0);
    if (ret)
        goto err_pwm;

    /*
     * Character Device 번호 할당
     */
    ret = alloc_chrdev_region(
        &fan->devno, 0, 1, DEVICE_NAME
    );
    if (ret)
        goto err_pwm;

    cdev_init(&fan->cdev, &fan_fops);
    fan->cdev.owner = THIS_MODULE;

    ret = cdev_add(&fan->cdev,
                   fan->devno, 1);
    if (ret)
        goto err_chrdev;

    fan->class = class_create(
        THIS_MODULE, CLASS_NAME
    );

    if (IS_ERR(fan->class)) {
        ret = PTR_ERR(fan->class);
        goto err_cdev;
    }

    fan->device = device_create(
        fan->class,
        &pdev->dev,
        fan->devno,
        NULL,
        DEVICE_NAME
    );

    if (IS_ERR(fan->device)) {
        ret = PTR_ERR(fan->device);
        goto err_class;
    }

    platform_set_drvdata(pdev, fan);

    dev_info(&pdev->dev,
             "/dev/fan_motor created\n");

    return 0;

err_class:
    class_destroy(fan->class);

err_cdev:
    cdev_del(&fan->cdev);

err_chrdev:
    unregister_chrdev_region(fan->devno, 1);

err_pwm:
    fan_apply_duty(fan, 0);
    pwm_put(fan->pwm);

    return ret;
}

/* ---------------------------------------
 * remove()
 * --------------------------------------- */
static int fan_remove(struct platform_device *pdev)
{
    struct fan_device *fan;

    fan = platform_get_drvdata(pdev);

    mutex_lock(&fan->lock);

    fan_apply_duty(fan, 0);

    mutex_unlock(&fan->lock);

    device_destroy(fan->class, fan->devno);
    class_destroy(fan->class);

    cdev_del(&fan->cdev);

    unregister_chrdev_region(fan->devno, 1);

    pwm_put(fan->pwm);

    dev_info(&pdev->dev,
             "fan_motor removed\n");

    return 0;
}

/* ---------------------------------------
 * Platform Driver
 * --------------------------------------- */
static struct platform_driver motor_driver = {
    .probe  = fan_probe,
    .remove = fan_remove,
    .driver = {
        .name = DEVICE_NAME,
    },
};

static struct platform_device *motor_pdev;

/* ---------------------------------------
 * module_init()
 * --------------------------------------- */
static int __init motor_init(void)
{
    int ret;

    pr_info("fan_motor: module init\n");

    ret = platform_driver_register(&motor_driver);
    if (ret)
        return ret;

    motor_pdev = platform_device_register_simple(
        DEVICE_NAME, -1, NULL, 0
    );

    if (IS_ERR(motor_pdev)) {
        ret = PTR_ERR(motor_pdev);

        platform_driver_unregister(&motor_driver);

        return ret;
    }

    /*
     * probe() 실행 여부 확인
     */
    if (!platform_get_drvdata(motor_pdev)) {

        pr_err("fan_motor: probe failed\n");

        platform_device_unregister(motor_pdev);
        platform_driver_unregister(&motor_driver);

        return -ENODEV;
    }

    return 0;
}

/* ---------------------------------------
 * module_exit()
 * --------------------------------------- */
static void __exit motor_exit(void)
{
    platform_device_unregister(motor_pdev);

    platform_driver_unregister(&motor_driver);

    pr_info("fan_motor: module exit\n");
}

module_init(motor_init);
module_exit(motor_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Smart Fan Project");
MODULE_DESCRIPTION("Jetson PWM Fan Motor Platform Driver");
