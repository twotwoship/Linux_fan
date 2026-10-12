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
	{ 0x147e4610, "i2c_smbus_read_byte_data" },
	{ 0xb4ba4bcd, "param_ops_int" },
	{ 0xffe4cb28, "i2c_del_driver" },
	{ 0xeaf27a3a, "i2c_smbus_read_i2c_block_data" },
	{ 0xb13205cf, "i2c_smbus_write_byte_data" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x277f7b23, "i2c_put_adapter" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xdcb764ad, "memset" },
	{ 0x4b0a3f52, "gic_nonsecure_priorities" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0xdbdf78ac, "i2c_new_client_device" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x7113fcab, "i2c_unregister_device" },
	{ 0x45dad54b, "i2c_register_driver" },
	{ 0x2b25d7a3, "cdev_add" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x1d24c881, "___ratelimit" },
	{ 0x92997ed8, "_printk" },
	{ 0x1e49e99f, "cpu_hwcaps" },
	{ 0x31909811, "cpu_hwcap_keys" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0xcff0b5f3, "i2c_get_adapter" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("i2c:bmp180_fan");
