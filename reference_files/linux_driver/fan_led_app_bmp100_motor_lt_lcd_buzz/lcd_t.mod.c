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
	{ 0xffe4cb28, "i2c_del_driver" },
	{ 0x45dad54b, "i2c_register_driver" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xcf8ebc79, "bmp180_get_data" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0x2d3385d3, "system_wq" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0xac49e8e8, "misc_register" },
	{ 0xc3055d20, "usleep_range_state" },
	{ 0xf9a482f9, "msleep" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x1e49e99f, "cpu_hwcaps" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x31909811, "cpu_hwcap_keys" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0x6188fcf, "_dev_err" },
	{ 0x1d24c881, "___ratelimit" },
	{ 0xc365546a, "i2c_transfer_buffer_flags" },
	{ 0x92997ed8, "_printk" },
	{ 0x287f6da0, "misc_deregister" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x1fdc7df2, "_mcount" },
	{ 0x4b0a3f52, "gic_nonsecure_priorities" },
};

MODULE_INFO(depends, "bmp180");

MODULE_ALIAS("of:N*T*Cmycompany,lcd_t");
MODULE_ALIAS("of:N*T*Cmycompany,lcd_tC*");
MODULE_ALIAS("i2c:lcd_t");
