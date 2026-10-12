#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/pwm.h>
#include <linux/workqueue.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>

// 문자 장치 번호 및 객체
static dev_t devt;
static struct cdev *buzzer_cdev;

// 사용자 프로그램의 중복 open 방지
static atomic_t device_in_use = ATOMIC_INIT(0);
// 모듈 종료 시 재생 중단 요청 
static atomic_t stop_requested = ATOMIC_INIT(0);

// 부저는 기존 물리 핀 33 PWM 사용.
static int pwm_id = 2;

module_param(pwm_id, int, 0444);
MODULE_PARM_DESC(pwm_id, "global PWM number for the selected buzzer pin");
// PWM 장치와 알림음 재생 작업 
static struct pwm_device* buzzer_pwm;
static struct work_struct buzzer_work;

//이벤트 접수 상태 보호 
static DEFINE_SPINLOCK(event_lock);
static bool accepting_events;

// FND에서 직접 호출할 함수 
int buzzer_fnd_complete(int value);

// PWM 설정은 초기화, 단일 worker, worker 종료 후 exit에서만 수행한다.
//지정한 주파수로 PWM 출력 
static int buzzer_set_tone(unsigned int hz)
{
    struct pwm_state state;
    u64 period_ns;

    if (hz == 0)
        return -EINVAL;

    // 주기(ns) = 1초(ns) / 주파수(Hz) 
    period_ns = DIV_ROUND_CLOSEST_ULL(1000000000ULL, hz);

    pwm_get_state(buzzer_pwm, &state);

    state.period = period_ns;
    state.duty_cycle = period_ns / 2;
    state.enabled = true;

    return pwm_apply_state(buzzer_pwm, &state);
}

// PWM 출력 중지 
static int buzzer_silence(void)
{
    struct pwm_state state;
    int ret;

    pwm_get_state(buzzer_pwm, &state);

    /*
     * 초기 PWM 상태의 period가 0인 경우
     * 유효한 주기를 먼저 설정한다.
     */
    if (state.period == 0) {
        state.period =
            DIV_ROUND_CLOSEST_ULL(1000000000ULL, 2093);
    }

    state.duty_cycle = 0;
    state.enabled = false;

    ret = pwm_apply_state(buzzer_pwm, &state);

    if (ret) {
        pr_err_ratelimited(
            "buzzer: PWM disable failed (%d)\n", ret);
    }

    return ret;
}

//지정 시간 동안 기다린다.10ms 단위로 종료 요청을 확인한다.
static bool buzzer_wait(unsigned int duration_ms)
{
    unsigned int elapsed = 0;

    while (elapsed < duration_ms) {
        unsigned int step;

        if (atomic_read(&stop_requested))
            return false;

        step = min(10U, duration_ms - elapsed);

        msleep(step);
        elapsed += step;
    }

    return !atomic_read(&stop_requested);
}

static void buzzer_play_worker(struct work_struct *work)
{
    static const struct {
        unsigned int hz;
        unsigned int duration_ms;
    } notes[] = {
        {2093, 120}, {2637, 120}, {2093, 120}, {2637, 350}
    };
    size_t i;
    int ret;
    (void)work;
    for (i = 0; i < ARRAY_SIZE(notes); i++) {
        if (atomic_read(&stop_requested))
            break;
        ret = buzzer_set_tone(notes[i].hz);
        if (ret) {
            pr_err_ratelimited("buzzer: PWM tone failed (%d)\n", ret);
            break;
        }
        // 현재 음의 재생 시간만큼 대기 
        if (!buzzer_wait(notes[i].duration_ms))
            break;

        // 음 사이에는 출력을 끈다. 
        if (buzzer_silence())
            break;

        //마지막 음을 제외하고 30ms 간격 
        if (i + 1 < ARRAY_SIZE(notes)) {
            if (!buzzer_wait(30))
                break;
        }
    }
    //정상 완료 또는 중단 시 출력 정지
    buzzer_silence();
}

 //value == 1: 알림음 재생 요청
 //value == 0: 아무 동작 없음
int buzzer_fnd_complete(int value)
{
    unsigned long flags;
    int ret = 0;

    if (value == 0)
        return 0;

    if (value != 1)
        return -EINVAL;

    spin_lock_irqsave(&event_lock, flags);

    if (!accepting_events) {
        ret = -ENODEV;
    }
    else {
        queue_work(system_wq, &buzzer_work);
    }

    spin_unlock_irqrestore(&event_lock, flags);

    return ret;
}

EXPORT_SYMBOL_GPL(buzzer_fnd_complete);

// 테스트 프로그램에서 장치를 열 때 호출 
static int buzzer_open(struct inode* inode, struct file* file)
{
    (void)inode;
    (void)file;

    if (atomic_cmpxchg(&device_in_use, 0, 1))
        return -EBUSY;

    return 0;
}

//테스트 프로그램에서 장치를 닫을 때 호출 
static int buzzer_release(struct inode* inode, struct file* file)
{
    (void)inode;
    (void)file;

    atomic_set(&device_in_use, 0);

    return 0;
}

//기존 사용자 공간 테스트 인터페이스 유지.
 //문자열이 아닌 native int 값 하나를 받는다.
static ssize_t buzzer_write(struct file* file,
    const char __user* buf,
    size_t count,
    loff_t* ppos)
{
    int value;
    int ret;

    (void)file;
    (void)ppos;

    if (count != sizeof(value))
        return -EINVAL;

    if (copy_from_user(&value, buf, sizeof(value)))
        return -EFAULT;

    ret = buzzer_fnd_complete(value);

    if (ret)
        return ret;

    return sizeof(value);
}

/* 읽기 기능은 제공하지 않는다. */
static ssize_t buzzer_read(struct file* file,
    char __user* buf,
    size_t count,
    loff_t* ppos)
{
    (void)file;
    (void)buf;
    (void)count;
    (void)ppos;

    return -EOPNOTSUPP;
}

static const struct file_operations buzzer_fops = {
    .owner = THIS_MODULE,
    .open = buzzer_open,
    .release = buzzer_release,
    .write = buzzer_write,
    .read = buzzer_read,
    .llseek = no_llseek,
};

//모듈 로드 시 초기화 
static int __init buzzer_init(void)
{
    unsigned long flags;
    int ret;

    if (pwm_id < 0)
        return -EINVAL;

    atomic_set(&device_in_use, 0);
    atomic_set(&stop_requested, 0);
    accepting_events = false;

    INIT_WORK(&buzzer_work, buzzer_play_worker);

    //문자 장치 번호 할당 자동
    ret = alloc_chrdev_region(&devt, 0, 1, "buzz_t");

    if (ret)
        return ret;

    //문자 장치 객체 할당 
    buzzer_cdev = cdev_alloc();

    if (!buzzer_cdev) {
        ret = -ENOMEM;
        goto err_region;
    }

    buzzer_cdev->ops = &buzzer_fops;
    buzzer_cdev->owner = THIS_MODULE;

    //부저 PWM 장치 요청 
    buzzer_pwm = pwm_request(pwm_id, "fan-buzzer");

    if (IS_ERR(buzzer_pwm)) {
        ret = PTR_ERR(buzzer_pwm);
        pr_err("buzzer: PWM request failed (%d)\n", ret);
        goto err_cdev;
    }

    //초기 상태는 무음 
    ret = buzzer_silence();

    if (ret)
        goto err_pwm;

    //문자 장치 등록 
    ret = cdev_add(buzzer_cdev, devt, 1);

    if (ret)
        goto err_pwm;

    //초기화 완료 후 이벤트 접수 허용 
    spin_lock_irqsave(&event_lock, flags);
    accepting_events = true;
    spin_unlock_irqrestore(&event_lock, flags);

    pr_info("buzzer: major=%u minor=%u PWM=%d\n",
        MAJOR(devt), MINOR(devt), pwm_id);

    return 0;

err_pwm:
    pwm_free(buzzer_pwm);

err_cdev:
    kobject_put(&buzzer_cdev->kobj);

err_region:
    unregister_chrdev_region(devt, 1);

    return ret;
}

//모듈 제거 시 정리 
static void __exit buzzer_exit(void)
{
    unsigned long flags;

    //새로운 재생 요청 차단 
    spin_lock_irqsave(&event_lock, flags);
    accepting_events = false;
    atomic_set(&stop_requested, 1);
    spin_unlock_irqrestore(&event_lock, flags);

    cdev_del(buzzer_cdev);

    //재생 작업이 끝난 다음 PWM 해제 
    cancel_work_sync(&buzzer_work);

    buzzer_silence();
    pwm_free(buzzer_pwm);

    unregister_chrdev_region(devt, 1);

    pr_info("buzzer: unloaded\n");
}

module_init(buzzer_init);
module_exit(buzzer_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Fan PWM buzzer called directly by FND");