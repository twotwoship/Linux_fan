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
	{ 0x2d3385d3, "system_wq" },
	{ 0x7d4e4e48, "cdev_del" },
	{ 0xb66ffcd7, "cdev_init" },
	{ 0x1fdc7df2, "_mcount" },
	{ 0x47229b5c, "gpio_request" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x9a550c00, "gpio_to_desc" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xa955d91, "__platform_driver_register" },
	{ 0x4b0a3f52, "gic_nonsecure_priorities" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0xd4d50f84, "gpiod_direction_output_raw" },
	{ 0x6188fcf, "_dev_err" },
	{ 0x2b25d7a3, "cdev_add" },
	{ 0x6881d240, "_dev_info" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x1e49e99f, "cpu_hwcaps" },
	{ 0x31909811, "cpu_hwcap_keys" },
	{ 0xfe990052, "gpio_free" },
	{ 0xf193393, "gpiod_set_raw_value" },
	{ 0x46d3d6e4, "platform_driver_unregister" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
};

MODULE_INFO(depends, "");

