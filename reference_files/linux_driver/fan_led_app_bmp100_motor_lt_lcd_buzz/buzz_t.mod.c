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
	{ 0xa3b3abe5, "kobject_put" },
	{ 0xf774dff8, "cdev_alloc" },
	{ 0x2d3385d3, "system_wq" },
	{ 0x7d4e4e48, "cdev_del" },
	{ 0xf9a482f9, "msleep" },
	{ 0x1fdc7df2, "_mcount" },
	{ 0xb4ba4bcd, "param_ops_int" },
	{ 0xc098597f, "pwm_apply_state" },
	{ 0x2eed8fd0, "no_llseek" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x3c12dfe, "cancel_work_sync" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x4b0a3f52, "gic_nonsecure_priorities" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x55b5ce0f, "pwm_request" },
	{ 0xd5920837, "pwm_free" },
	{ 0x2b25d7a3, "cdev_add" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x1d24c881, "___ratelimit" },
	{ 0x92997ed8, "_printk" },
	{ 0x1e49e99f, "cpu_hwcaps" },
	{ 0x31909811, "cpu_hwcap_keys" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
};

MODULE_INFO(depends, "");

