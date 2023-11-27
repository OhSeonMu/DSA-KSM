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
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xda251cd5, "filp_open" },
	{ 0x65487097, "__x86_indirect_thunk_rax" },
	{ 0x5be18095, "filp_close" },
	{ 0x858465e6, "param_set_int" },
	{ 0xd29bcf69, "kthread_stop" },
	{ 0x32887dd3, "dma_alloc_attrs" },
	{ 0xb7106351, "kmalloc_caches" },
	{ 0x3f57e9c4, "kmalloc_trace" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0xa4b49a3, "dma_map_page_attrs" },
	{ 0x41ed3709, "get_random_bytes" },
	{ 0x171c5f9a, "kthread_create_on_node" },
	{ 0xf77189d0, "kthread_bind" },
	{ 0xacade4a2, "wake_up_process" },
	{ 0x4c9d28b0, "phys_base" },
	{ 0x4d41ff1c, "dma_unmap_page_attrs" },
	{ 0x8bd1cac2, "dma_free_attrs" },
	{ 0xa19b956, "__stack_chk_fail" },
	{ 0x48d3fa27, "kmalloc_large_node" },
	{ 0xf9a482f9, "msleep" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x1b94fc43, "param_get_int" },
	{ 0x78d3735f, "param_ops_int" },
	{ 0x122c3a7e, "_printk" },
	{ 0xd5db4eda, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "B0D0FE3D7CF68D12AC7EE86");
