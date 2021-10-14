#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

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
	{ 0x3364d393, "module_layout" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x80a1a9e4, "cdev_del" },
	{ 0x408bae9a, "cdev_add" },
	{ 0xd8135e1a, "cdev_init" },
	{ 0x6c3c5772, "cdev_alloc" },
	{ 0x3fd78f3b, "register_chrdev_region" },
	{ 0xc959d152, "__stack_chk_fail" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "227961EE9D3FD3ECD2DE26D");
