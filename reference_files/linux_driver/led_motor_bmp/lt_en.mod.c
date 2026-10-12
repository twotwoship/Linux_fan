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
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x7d4e4e48, "cdev_del" },
	{ 0xedc03953, "iounmap" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x5b04e4a6, "gpiod_to_irq" },
	{ 0x9a550c00, "gpio_to_desc" },
	{ 0x6b4b2933, "__ioremap" },
	{ 0xaf56600a, "arm64_use_ng_mappings" },
	{ 0x2b25d7a3, "cdev_add" },
	{ 0xf774dff8, "cdev_alloc" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x92540fbf, "finish_wait" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0x1000e51, "schedule" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x1e49e99f, "cpu_hwcaps" },
	{ 0x6bc39b29, "try_module_get" },
	{ 0xf5a8334f, "module_put" },
	{ 0x4b0a3f52, "gic_nonsecure_priorities" },
	{ 0x92997ed8, "_printk" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0x31909811, "cpu_hwcap_keys" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x1fdc7df2, "_mcount" },
};

MODULE_INFO(depends, "");

