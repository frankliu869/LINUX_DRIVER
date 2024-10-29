#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");

static int __init hello_world_init(void)
{
    printk("hello kernel !\n");
    return 0;
}

static void __exit hello_world_exit(void) 
{
	printk("Goodbye, Kernel\n");
}

module_init(hello_world_init);
module_exit(hello_world_exit);