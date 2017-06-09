#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/syscore_ops.h>
#include <linux/irq.h>
#include <linux/fcntl.h>
#include <linux/spinlock.h>

#include <linux/fs.h>
#include <asm/uaccess.h>    // for put_user

// include RPi harware specific constants
//#include <mach/hardware.h>
#include "dht.h"

#define DRIVER_AUTHOR "Peter Jay Salzman <p@dirac.org>"
#define DRIVER_DESC   "A sample driver"

static int gpio_pin = 0;        //Default GPIO pin
static int driverno = 80;        //Default driver number
static int format = 0;        //Default result format

volatile unsigned *gpio;

// Possible valid GPIO pins
int valid_gpio_pins[] = { 0, 1, 4, 8, 7, 9, 10, 11, 14, 15, 17, 18, 21, 22, 23,    24, 25 };

// Initialise GPIO memory
static int init_port(void)
{
    // reserve GPIO memory region.
    if (request_mem_region(GPIO_BASE, SZ_4K, DHT11_DRIVER_NAME) == NULL) {
        printk(KERN_ERR DHT11_DRIVER_NAME ": unable to obtain GPIO I/O memory address\n");
        return -EBUSY;
    }

    // remap the GPIO memory
    if ((gpio = ioremap_nocache(GPIO_BASE, SZ_4K)) == NULL) {
        printk(KERN_ERR DHT11_DRIVER_NAME ": failed to map GPIO I/O memory\n");
        return -EBUSY;
    }

    return 0;
}

static int __init dht_init(void) {
    // register_chrdev
//    request_mem_region()
//    ioremap_nocache() 實體位置映射到 kernel virtual memory

    int result;
    int i;

    // check for valid gpio pin number
    result = 0;
    for(i = 0; (i < ARRAY_SIZE(valid_gpio_pins)) && (result != 1); i++) {
        if(gpio_pin == valid_gpio_pins[i])
            result++;
    }

    if (result != 1) {
        result = -EINVAL;
        printk(KERN_ERR DHT11_DRIVER_NAME ": invalid GPIO pin specified!\n");
        return result;
    }

    result = register_chrdev(driverno, DHT11_DRIVER_NAME, &fops);

    if (result < 0) {
      printk(KERN_ALERT DHT11_DRIVER_NAME "Registering dht11 driver failed with %d\n", result);
      return result;
    }

    printk(KERN_INFO DHT11_DRIVER_NAME ": driver registered!\n");

    result = init_port();
    if (result < 0)
        return result;

    return 0;
}

/*
 * This function is called when the module is unloaded
 */
static void __exit dht_exit(void) {
    //iounmap()
    //release_mem_region()
    //unregister_chrdev()

    // release mapped memory and allocated region
    if(gpio != NULL) {
        iounmap(gpio);
        release_mem_region(GPIO_BASE, SZ_4K);
        printk(DHT11_DRIVER_NAME ": cleaned up resources\n");
    }

    // Unregister the driver
    unregister_chrdev(driverno, DHT11_DRIVER_NAME);
    printk(DHT11_DRIVER_NAME ": cleaned up module\n");

}

module_init(dht_init);
module_exit(dht_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);        /* What does this module do */
MODULE_SUPPORTED_DEVICE("testdevice");

module_param(format, int, S_IRUGO);
MODULE_PARM_DESC(gpio_pin, "Format of output");
module_param(gpio_pin, int, S_IRUGO);
MODULE_PARM_DESC(gpio_pin, "GPIO pin to use");
module_param(driverno, int, S_IRUGO);
MODULE_PARM_DESC(driverno, "Driver handler major value");