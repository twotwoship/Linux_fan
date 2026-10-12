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
        '#include <assert.h>\n#include <errno.h>\ntypedef unsigned int __u32;',
        header('fan_common.h'), header('fan_motor.h'), header('led_ctl.h'),
        'struct fan_device { __u32 mode, duty; };',
        'static int fan_apply_duty(struct fan_device *fan, __u32 duty) '
        '{ fan->duty = duty; return 0; }',
        function(folder / 'fan_motor.c', 'fan_set_mode'),
        function(folder / 'fan_motor.c', 'fan_temp_to_duty'),
        function(folder / 'fan_app.c', 'next_mode'),
        function(BUILD / 'led_b.c', 'duty_to_count'),
        'static int target_count, led_lock, anim_work;',
        '#define mutex_lock(lock) ((void)(lock))\n'
        '#define mutex_unlock(lock) ((void)(lock))\n'
        '#define schedule_delayed_work(work, delay) ((void)(work), (void)(delay))',
        function(BUILD / 'led_b.c', 'led_set_duty'),
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
            assert(led_set_duty(fan.duty) == 0);
            assert(target_count == 1);
            for (int i = 0; i < 6; i++) {
                assert(mode == (enum fan_mode)i);
                assert(fan_set_mode(&fan, mode) == 0);
                assert(fan.duty == expected_duty[i]);
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
            return 0;
        }''',
    ])

    with tempfile.TemporaryDirectory() as directory:
        test = Path(directory) / 'led_mapping.c'
        binary = Path(directory) / 'led_mapping'
        test.write_text(source)
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                        str(test), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print(f'PASS {folder.name}: button cycle, manual/AUTO LED mapping, OFF, invalid duties')


if __name__ == '__main__':
    for folder in (HERE, BUILD):
        test_mapping(folder)
