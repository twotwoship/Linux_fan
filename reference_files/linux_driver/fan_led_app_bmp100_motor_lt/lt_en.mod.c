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
	{ 0xf774dff8, "cdev_alloc" },
	{ 0x7d4e4e48, "cdev_del" },
	{ 0x1fdc7df2, "_mcount" },
	{ 0x13d0adf7, "__kfifo_out" },
	{ 0x9a550c00, "gpio_to_desc" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x4b0a3f52, "gic_nonsecure_priorities" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x6b4b2933, "__ioremap" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x2b25d7a3, "cdev_add" },
	{ 0xf5a8334f, "module_put" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x1000e51, "schedule" },
	{ 0x92997ed8, "_printk" },
	{ 0x1e49e99f, "cpu_hwcaps" },
	{ 0x31909811, "cpu_hwcap_keys" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x5b04e4a6, "gpiod_to_irq" },
	{ 0xaf56600a, "arm64_use_ng_mappings" },
	{ 0xedc03953, "iounmap" },
	{ 0x92540fbf, "finish_wait" },
	{ 0xf23fcb99, "__kfifo_in" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x6bc39b29, "try_module_get" },
	{ 0xc1514a3b, "free_irq" },
};

MODULE_INFO(depends, "");

