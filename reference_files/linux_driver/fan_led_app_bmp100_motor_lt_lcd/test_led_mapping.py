"""보드 없이 실제 C 함수의 버튼 순서 및 모터/LED 매핑을 검증한다."""
from pathlib import Path
import re
import subprocess
import tempfile


HERE = Path(__file__).resolve().parent
BUILD = HERE.with_name(HERE.name + "_buzz")


def header(name):
    return re.sub(r'^#include.*$', '', (BUILD / name).read_text(), flags=re.M)


def function(path, name):
    source = path.read_text()
    start = re.search(r'^(?:static )?\w+[ \t]+[^\n]*\b' + name + r'\(', source, re.M).start()
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def test_mapping(folder):
    source = '\n'.join([
        '#include <assert.h>\n#include <errno.h>\n#include <stdbool.h>\n#include <stddef.h>\n'
        'typedef unsigned int __u32;\n'
        '#define pr_info(...) ((void)0)\n#define pr_warn_ratelimited(...) ((void)0)',
        header('fan_common.h'), header('fan_motor.h'), header('led_ctl.h'),
        header('led_b.h'),
        'struct pwm_device { int unused; };\n'
        'struct fan_device { struct pwm_device *pwm; __u32 mode, duty; };',
        'static int fan_apply_duty(struct fan_device *fan, __u32 duty);',
        function(folder / 'fan_motor.c', 'fan_set_mode'),
        function(folder / 'fan_motor.c', 'fan_temp_to_duty'),
        function(folder / 'fan_app.c', 'next_mode'),
        function(BUILD / 'led_b.c', 'duty_to_count'),
        'static int target_count, cur_count, led_lock, anim_work;\n'
        'struct work_struct { int unused; };\n'
        'static struct led_bar_pdata pdata = {{0,1,2,3,4,5,6}, 0};\n'
        'static struct led_bar_pdata *pd = &pdata;\n'
        'static int gpio_output[LED_NUM];\n'
        'static void gpio_set_value(int gpio, int value) { gpio_output[gpio] = value; }\n'
        '#define ANIM_STEP_MS 80\n#define msecs_to_jiffies(ms) (ms)',
        '#define mutex_lock(lock) ((void)(lock))\n'
        '#define mutex_unlock(lock) ((void)(lock))\n'
        '#define schedule_delayed_work(work, delay) ((void)(work), (void)(delay))',
        function(BUILD / 'led_b.c', 'led_set_duty'),
        function(BUILD / 'led_b.c', 'led_apply'),
        function(BUILD / 'led_b.c', 'anim_fn'),
        'struct pwm_state { unsigned long long period, duty_cycle; int polarity; bool enabled; };\n'
        'static struct pwm_state applied_pwm;\nstatic int pwm_error;\n'
        '#define PWM_PERIOD_NS 50000ULL\n#define PWM_POLARITY_NORMAL 0\n'
        'static void pwm_get_state(struct pwm_device *pwm, struct pwm_state *state) '
        '{ (void)pwm; *state = applied_pwm; }\n'
        'static int pwm_apply_state(struct pwm_device *pwm, const struct pwm_state *state) '
        '{ (void)pwm; if (!pwm_error) applied_pwm = *state; return pwm_error; }\n'
        'static void fan_direction_forward(void) {}\nstatic void fan_direction_stop(void) {}\n'
        '#define symbol_get(symbol) (symbol)\n#define symbol_put(symbol) ((void)0)',
        function(folder / 'fan_motor.c', 'fan_apply_duty'),
        'typedef int irqreturn_t;\n#define IRQ_HANDLED 1\n#define DEBOUNCE_TIME_MS 20\n'
        '#define GPIO_INPUT 8\n#define atomic_inc(p) (++*(p))\n'
        '#define wake_up_interruptible(p) ((void)(p))\n#define printk(...) ((void)0)\n'
        'static int key_work, key_event_count, key_wait_queue, key_level = 1;\n'
        'static unsigned long gpio_key_base;\nstatic bool key_pressed;\n'
        'static long now_ms, key_due_ms = -1;\nstatic void *system_wq;\n'
        'static unsigned int ioread32(void *address) { (void)address; return key_level; }\n'
        'static void mod_delayed_work(void *queue, int *work, int delay) {\n'
        '    (void)queue; (void)work; key_due_ms = now_ms + delay;\n}',
        function(folder / 'lt_en.c', 'key_debounce_work'),
        function(folder / 'lt_en.c', 'key_isr'),
        '''static void advance_time(long time) {
            if (key_due_ms >= 0 && key_due_ms <= time) {
                now_ms = key_due_ms;
                key_due_ms = -1;
                key_debounce_work(NULL);
            }
            now_ms = time;
        }
        static void key_edge(long time, int level) {
            advance_time(time);
            key_level = level;
            key_isr(0, NULL);
        }''',
        '''int main(void) {
            struct fan_device fan = {0};
            enum fan_mode mode = FAN_OFF;
            unsigned int expected_duty[] = {0, 0, 65, 75, 85, 95};
            int expected_leds[] = {0, 0, 1, 3, 5, 7};
            int temperatures[] = {257, 258, 264, 265, 272, 273};
            int auto_leds[] = {1, 3, 3, 5, 5, 7};
            assert(led_set_duty(75) == 0);
            assert(target_count == 3);
            assert(led_set_duty(70) == -EINVAL);
            assert(target_count == 3);
            assert(fan_set_mode(&fan, FAN_FINE) == 0);
            assert(target_count == 1);
            for (int i = 0; i < 6; i++) {
                assert(mode == (enum fan_mode)i);
                assert(fan_set_mode(&fan, mode) == 0);
                assert(fan.duty == expected_duty[i]);
                assert(applied_pwm.duty_cycle == PWM_PERIOD_NS * expected_duty[i] / 100);
                assert(target_count == expected_leds[i]);
                assert(duty_to_count(fan.duty) == expected_leds[i]);
                mode = next_mode(mode);
                assert(duty_to_count(fan_temp_to_duty(temperatures[i])) == auto_leds[i]);
            }
            assert(mode == FAN_OFF);
            assert(fan_set_mode(&fan, FAN_OFF) == 0);
            assert(duty_to_count(fan.duty) == 0);
            assert(fan_set_mode(&fan, 6) == -EINVAL);
            for (unsigned int duty = 0; duty <= 101; duty++) {
                if (duty != 0 && duty != 65 && duty != 75 && duty != 85 && duty != 95)
                    assert(duty_to_count(duty) == -EINVAL);
            }
            assert(duty_to_count((unsigned int)-1) == -EINVAL);
            /* Check every LED transition and both output polarities. */
            for (int polarity = 0; polarity <= 1; polarity++) {
                pd->active_low = polarity;
                for (int from = 0; from < 6; from++) {
                    for (int to = 0; to < 6; to++) {
                        cur_count = expected_leds[from];
                        assert(fan_set_mode(&fan, to) == 0);
                        for (int step = 0; step < LED_NUM; step++) anim_fn(NULL);
                        assert(cur_count == expected_leds[to]);
                        for (int pin = 0; pin < LED_NUM; pin++)
                            assert(gpio_output[pin] == ((pin < cur_count) ^ polarity));
                    }
                }
            }
            /* AUTO temperature changes and the following manual mode use the same real path. */
            for (int i = 0; i < 6; i++) {
                assert(fan_set_mode(&fan, FAN_AUTO) == 0);
                assert(fan_apply_duty(&fan, fan_temp_to_duty(temperatures[i])) == 0);
                assert(target_count == auto_leds[i]);
                assert(fan_set_mode(&fan, FAN_FINE) == 0);
                assert(target_count == 1);
                for (int step = 0; step < LED_NUM; step++) anim_fn(NULL);
                assert(cur_count == 1);
            }
            pwm_error = -EIO;
            assert(fan_set_mode(&fan, FAN_LOW) == -EIO);
            assert(fan.mode == FAN_FINE && fan.duty == 65 && target_count == 1);
            pwm_error = 0;
            /* One press from AUTO, including press and release bounce. */
            key_edge(100, 0); key_edge(102, 1); key_edge(104, 0);
            advance_time(124);
            assert(key_event_count == 1);
            key_edge(500, 1); key_edge(502, 0); key_edge(504, 1);
            advance_time(524);
            assert(key_event_count == 1);
            mode = FAN_AUTO;
            for (int event = 0; event < key_event_count; event++) mode = next_mode(mode);
            assert(mode == FAN_FINE);
            assert(fan_set_mode(&fan, mode) == 0);
            assert(target_count == 1);
            /* A real second press still works; a short glitch does not. */
            key_edge(600, 0); advance_time(620);
            assert(key_event_count == 2);
            key_edge(700, 1); advance_time(720);
            key_edge(800, 0); key_edge(805, 1); advance_time(825);
            assert(key_event_count == 2);
            return 0;
        }''',
    ])

    with tempfile.TemporaryDirectory() as directory:
        test = Path(directory) / 'led_mapping.c'
        binary = Path(directory) / 'led_mapping'
        test.write_text(source)
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter',
                        str(test), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print(f'PASS {folder.name}: button debounce/cycle, manual/AUTO LED mapping, GPIO output, OFF, invalid duties')


if __name__ == '__main__':
    for folder in (HERE, BUILD):
        test_mapping(folder)
