#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x5063314d, "module_layout" },
	{ 0x91053684, "param_array_ops" },
	{ 0x8fded1ef, "param_ops_bool" },
	{ 0xb4ba4bcd, "param_ops_int" },
	{ 0x2eed8fd0, "no_llseek" },
	{ 0xf9b9c6b5, "platform_device_unregister" },
	{ 0x46d3d6e4, "platform_driver_unregister" },
	{ 0x80abe1d6, "platform_device_register_full" },
	{ 0xdcb764ad, "memset" },
	{ 0xa955d91, "__platform_driver_register" },
	{ 0x6188fcf, "_dev_err" },
	{ 0x6881d240, "_dev_info" },
	{ 0xac49e8e8, "misc_register" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xe6a53700, "devm_gpio_request_one" },
	{ 0xcefb0c9f, "__mutex_init" },
	{ 0xd8d38480, "devm_kmalloc" },
	{ 0x986aa9d5, "_cw_ccw_event" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x92997ed8, "_printk" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x1e49e99f, "cpu_hwcaps" },
	{ 0x31909811, "cpu_hwcap_keys" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0x2d3385d3, "system_wq" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x287f6da0, "misc_deregister" },
	{ 0xf193393, "gpiod_set_raw_value" },
	{ 0x9a550c00, "gpio_to_desc" },
	{ 0x1fdc7df2, "_mcount" },
	{ 0x4b0a3f52, "gic_nonsecure_priorities" },
};

MODULE_INFO(depends, "lt_en");

