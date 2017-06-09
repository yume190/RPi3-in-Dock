#include <linux/module.h>
#include <linux/kernel.h>
#include <mach/platform.h>

int init_module(void) {
	printk(KERN_INFO "Hello world 1.\n");
	
	printk(KERN_INFO "Base at : %d\n", GPIO_BASE);
	return 0;
}

void cleanup_module(void) {
	printk(KERN_INFO "Goodbye world 1.\n");
}