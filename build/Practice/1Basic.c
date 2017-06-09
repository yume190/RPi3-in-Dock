#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
/* Needed by all modules */
/* Needed for KERN_INFO */
/* Needed for the macros */
static int __init basic_init(void) {
	printk(KERN_INFO "Hello, world\n");
	return 0;
}

static void __exit basic_exit(void) {
	printk(KERN_INFO "Goodbye, world\n");
}

module_init(basic_init);
module_exit(basic_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yume");
MODULE_DESCRIPTION("Kernel Basic");        /* What does this module do */
MODULE_SUPPORTED_DEVICE("testdevice");