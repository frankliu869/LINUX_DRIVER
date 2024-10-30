#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

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
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
	{ 0x92997ed8, "_printk" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x732f401c, "cdev_init" },
	{ 0x24e991b7, "cdev_add" },
	{ 0x58cffff9, "class_create" },
	{ 0x16e6c990, "device_create" },
	{ 0xde276c1a, "cdev_del" },
	{ 0x77fcce42, "device_destroy" },
	{ 0x70dd78f1, "class_destroy" },
	{ 0xa6f3f0f1, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "A5B605B51DA340E8D7EF290");
