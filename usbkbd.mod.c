#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
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
	{ 0x94e7504, "input_allocate_device" },
	{ 0x501a1f79, "usb_alloc_urb" },
	{ 0xf812cff6, "memscan" },
	{ 0xd0d89c51, "device_set_wakeup_enable" },
	{ 0xbd88d6c, "usb_free_urb" },
	{ 0xa524530, "usb_alloc_coherent" },
	{ 0x656e4a6e, "snprintf" },
	{ 0x815ab7b1, "usb_register_driver" },
	{ 0x1ca0e1fd, "input_unregister_device" },
	{ 0x37a0cba, "kfree" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xc39d1720, "input_free_device" },
	{ 0x92997ed8, "_printk" },
	{ 0x8427cc7b, "_raw_spin_lock_irq" },
	{ 0x6751ece4, "input_register_device" },
	{ 0xa916b694, "strnlen" },
	{ 0x64cb8054, "usb_submit_urb" },
	{ 0x35092812, "_dev_info" },
	{ 0x43ab59a7, "_dev_err" },
	{ 0x96ace085, "usb_free_coherent" },
	{ 0x4b750f53, "_raw_spin_unlock_irq" },
	{ 0x3359e33b, "usb_deregister" },
	{ 0xf9c0b663, "strlcat" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x38533483, "input_event" },
	{ 0xfea8f0e1, "usb_kill_urb" },
	{ 0x9e1b9a34, "kmalloc_trace" },
	{ 0x754d539c, "strlen" },
	{ 0x98c7b76a, "kmalloc_caches" },
	{ 0xc2855f40, "module_layout" },
};

MODULE_INFO(depends, "usbcore");

MODULE_ALIAS("usb:v*p*d*dc*dsc*dp*ic03isc01ip01in*");

MODULE_INFO(srcversion, "A1A48B08311AD0068B0E88B");
