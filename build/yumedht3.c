/* dht11km.c
 *
 * dht11km - Device driver for reading values from DHT11 temperature and humidity sensor.
 *
 *             By default the DHT11 is connected to GPIO pin 0 (pin 3 on the GPIO connector)
 *           The Major version default is 80 but can be set via the command line.
 *             Command line parameters: gpio_pin=X - a valid GPIO pin value
 *                                      driverno=X - value for Major driver number
 *                                      format=X - format of the output from the sensor
 *
 * Usage:
 *        Load driver:     insmod ./dht11km.ko <optional variables>
 *                i.e.       insmod ./dht11km.ko gpio_pin=2 format=3
 *
 *          Set up device file to read from (i.e.):
 *                        mknod /dev/dht11 c 80 0
 *                        mknod /dev/myfile c <driverno> 0    - to set the output to your own file and driver number
 *
 *          To read the values from the sensor: cat /dev/dht11
 *
 * Copyright (C) 2012 Nigel Morton <nigel@ntpworld.co.uk>
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
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
#include <asm/io.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <mach/platform.h>

/****************************************************************************/
/* Interrupts variables block                                               */
/****************************************************************************/
short int irq_any_gpio    = 0;

#define DHT11_DRIVER_NAME "dht11"
#define RBUF_LEN 256
#define SUCCESS 0
#define BUF_LEN 80        // Max length of the message from the device
// text below will be seen in 'cat /proc/interrupt' command
#define GPIO_ANY_GPIO_DESC           "Some gpio pin description"
// below is optional, used in more complex code, in our case, this could be
// NULL
#define GPIO_ANY_GPIO_DEVICE_DESC    "some_device"

// volatile unsigned *gpio;
struct GpioRegisters {
    uint32_t GPFSEL[6];							// 0~5
    uint32_t Reserved1;
    uint32_t GPSET[2];							// 7 8
    uint32_t Reserved2;
    uint32_t GPCLR[2];							// 10 11
    uint32_t Reserved3;
    uint32_t GPLEVEL[2];						// 13 14
    uint32_t Reserved4;
    uint32_t GPDETECT_STATUS[2];				// 16 17
    uint32_t Reserved5;
    uint32_t GPRISE_EDGE_DETECT_ENABLE[2];		// 19 20
    uint32_t Reserved6;
    uint32_t GPFALL_EDGE_DETECT_ENABLE[2];		// 21 22
};

struct GpioRegisters *s_pGpioRegisters;

// set GPIO pin g as input
// #define GPIO_DIR_INPUT(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
static void GPIO_DIR_INPUT(int GPIO) {
    int registerIndex = GPIO / 10;
    int bit = (GPIO % 10) * 3;
    int functionCode = 7;
    
    unsigned oldValue = s_pGpioRegisters->GPFSEL[registerIndex];
    unsigned mask = 0b111 << bit;
    // printk("Changing function of GPIO%d from %x to %x\n", GPIO, (oldValue >> bit) & 0b111, functionCode);
    s_pGpioRegisters->GPFSEL[registerIndex] = (oldValue & ~mask) | ((functionCode << bit) & mask);
}
// set GPIO pin g as output
// #define GPIO_DIR_OUTPUT(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
static void GPIO_DIR_OUTPUT(int GPIO) {
    int registerIndex = GPIO / 10;
    int bit = (GPIO % 10) * 3;
    int functionCode = 1;
    
    unsigned oldValue = s_pGpioRegisters->GPFSEL[registerIndex];
    unsigned mask = 0b111 << bit;
    // printk("Changing function of GPIO%d from %x to %x\n", GPIO, (oldValue >> bit) & 0b111, functionCode);
    s_pGpioRegisters->GPFSEL[registerIndex] = (oldValue & ~mask) | ((functionCode << bit) & mask);
}
// get logical value from gpio pin g
// #define GPIO_READ_PIN(g) (*(gpio+13) & (1<<(g))) && 1
static int GPIO_READ_PIN(int GPIO) {
    return s_pGpioRegisters->GPLEVEL[GPIO / 32] & (1<<GPIO) && 1;
}
// sets   bits which are 1 ignores bits which are 0
// #define GPIO_SET_PIN(g)    *(gpio+7) = 1<<g;
static void GPIO_SET_PIN(int GPIO) {
    s_pGpioRegisters->GPSET[GPIO / 32] = (1<<GPIO);
}
// clears bits which are 1 ignores bits which are 0
// #define GPIO_CLEAR_PIN(g) *(gpio+10) = 1<<g;
static void GPIO_CLEAR_PIN(int GPIO) {
    s_pGpioRegisters->GPCLR[GPIO / 32] = (1<<GPIO);
}
// Clear GPIO interrupt on the pin we use
// #define GPIO_INT_CLEAR(g) *(gpio+16) = (*(gpio+16) | (1<<g));
static void GPIO_INT_CLEAR(int GPIO) {
    s_pGpioRegisters->GPCLR[GPIO / 32] = s_pGpioRegisters->GPCLR[GPIO / 32] | (1<<GPIO);
}
// GPREN0 GPIO Pin Rising Edge Detect Enable/Disable
// #define GPIO_INT_RISING(g,v) *(gpio+19) = v ? (*(gpio+19) | (1<<g)) : (*(gpio+19) ^ (1<<g))
static void GPIO_INT_RISING(int GPIO, int v) {
    s_pGpioRegisters->GPRISE_EDGE_DETECT_ENABLE[GPIO / 32] = v ?
    s_pGpioRegisters->GPRISE_EDGE_DETECT_ENABLE[GPIO / 32] | (1<<GPIO) :
    s_pGpioRegisters->GPRISE_EDGE_DETECT_ENABLE[GPIO / 32] ^ (1<<GPIO);
}
// GPFEN0 GPIO Pin Falling Edge Detect Enable/Disable
// #define GPIO_INT_FALLING(g,v) *(gpio+22) = v ? (*(gpio+22) | (1<<g)) : (*(gpio+22) ^ (1<<g))
static void GPIO_INT_FALLING(int GPIO, int v) {
    s_pGpioRegisters->GPFALL_EDGE_DETECT_ENABLE[GPIO / 32] = v ?
    s_pGpioRegisters->GPFALL_EDGE_DETECT_ENABLE[GPIO / 32] | (1<<GPIO) :
    s_pGpioRegisters->GPFALL_EDGE_DETECT_ENABLE[GPIO / 32] ^ (1<<GPIO);
}

// Forward declarations
static int read_dht11(struct inode *, struct file *);
static int close_dht11(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static void clear_interrupts(void);

// Global variables are declared as static, so are global within the file.
static char msg[BUF_LEN];                // The msg the device will give when asked
static char *msg_Ptr;
static spinlock_t lock;

struct dht_control {
    int Device_Open;
    // spinlock_t lock;
    int bitcount;
    // int bytecount;
    int isstarted;
    struct timeval lasttv;
};

struct dht_data {
    char TemperatureInt;
    char TemperatureFloat;
    char HumidityInt;
    char HumidityFloat;
    char crc;
    struct dht_control control;
};

static struct dht_data dht;

// insmod paramaters
static int format = 3;        //Default result format
static int gpio_pin = 18;        //Default GPIO pin
static int driverno = 190;        //Default driver number

//Operations that can be performed on the device
static struct file_operations fops = {
    .read = device_read,
    .open = read_dht11,
    .release = close_dht11
};

static void resetDht(struct dht_data *dht) {
    dht->control.isstarted = 0;
    dht->control.bitcount = 0;
    // dht->control.bytecount = 0;
    dht->TemperatureInt = 0;
    dht->TemperatureFloat = 0;
    dht->HumidityInt = 0;
    dht->HumidityFloat = 0;
    dht->crc = 0;
} 

static int isDhtDataValid(struct dht_data *dht) {
    return (dht->TemperatureInt + dht->TemperatureFloat + dht->HumidityInt + dht->HumidityFloat == dht->crc) & 
           (dht->crc > 0);
}

static int readDhtData_time(struct dht_data *dht) {
    struct timeval tv;
    long deltv;
    int data = 0;
    // get current time
    do_gettimeofday(&tv);
    
    // get time since last interrupt in microseconds
    deltv = tv.tv_sec - dht->control.lasttv.tv_sec;
    
    data = (int) (deltv*1000000 + (tv.tv_usec - dht->control.lasttv.tv_usec));
    dht->control.lasttv = tv;    //Save last interrupt time

    return data;
}

static void processAndStoreByReadDhtDataTime(struct dht_data *dht, int signal, int time) {
    if((signal == 1)&(time > 40)) {
        dht->control.isstarted = 1;
        return; // IRQ_HANDLED;
    }
    
    if((signal == 0) & (dht->control.isstarted==1)) {
        if(time > 80)
            return; // IRQ_HANDLED;                                        //Start/spurious? signal
        if(time < 15)
            return; // IRQ_HANDLED;                                        //Spurious signal?
        if (time > 60) {//55
            char bit = 0x80 >> (dht->control.bitcount % 8);

            switch (dht->control.bitcount / 8) {
                case 0:
                    dht->TemperatureInt |= bit;
                    break;
                case 1:
                    dht->TemperatureFloat |= bit;
                    break;
                case 2:
                    dht->HumidityInt |= bit;
                    break;
                case 3:
                    dht->HumidityFloat |= bit;
                    break;
                case 4:
                    dht->crc |= bit;
                    break;
                default:
                    break;
            }
        }
        
        dht->control.bitcount++;//cobitcount++;
    }
}

// IRQ handler - where the timing takes place
static irqreturn_t irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
    unsigned long flags;
    int time;
    int signal;

    // disable hard interrupts (remember them in flag 'flags')
    local_irq_save(flags);
    
    // use the GPIO signal level
    signal = GPIO_READ_PIN(gpio_pin);
    
    /* reset interrupt */
    GPIO_INT_CLEAR(gpio_pin);
    
    time = readDhtData_time(&dht);
    processAndStoreByReadDhtDataTime(&dht, signal, time);
    
    // restore hard interrupts
    local_irq_restore(flags);
    
    return IRQ_HANDLED;
}

static int __init dht11_init_module(void)
{
    int result;
    s_pGpioRegisters = (struct GpioRegisters *)__io_address(GPIO_BASE);
    
    if (gpio_request(gpio_pin, GPIO_ANY_GPIO_DESC)) {
        printk("GPIO request faiure: %s\n", GPIO_ANY_GPIO_DESC);
        return -1;
    }
    
    if ( (irq_any_gpio = gpio_to_irq(gpio_pin)) < 0 ) {
        printk("GPIO to IRQ mapping faiure %s\n", GPIO_ANY_GPIO_DESC);
        return -1;
    }
    
    printk(KERN_NOTICE "Mapped int %d\n", irq_any_gpio);
    
    result = register_chrdev(driverno, DHT11_DRIVER_NAME, &fops);
    
    if (result < 0) {
        printk(KERN_ALERT DHT11_DRIVER_NAME "Registering dht11 driver failed with %d\n", result);
        return result;
    }
    
    printk(KERN_INFO DHT11_DRIVER_NAME ": driver registered! c %d\n",gpio_pin);
    
    return 0;
}

static void __exit dht11_exit_module(void) {
    // Unregister the driver
    unregister_chrdev(driverno, DHT11_DRIVER_NAME);
    
    gpio_free(gpio_pin);
    
    printk(DHT11_DRIVER_NAME ": cleaned up module\n");
}

static int setup_interrupts(void)
{
    int result;
    unsigned long flags;

    if ((result = request_irq(irq_any_gpio,
                    (irq_handler_t ) irq_handler,
                    IRQF_TRIGGER_FALLING,
                    GPIO_ANY_GPIO_DESC,
                    GPIO_ANY_GPIO_DEVICE_DESC))) {
        printk("Irq Request failure\n");
        return result;
    }
    
    spin_lock_irqsave(&lock, flags);
    
    // GPREN0 GPIO Pin Rising Edge Detect Enable
    GPIO_INT_RISING(gpio_pin, 1);
    // GPFEN0 GPIO Pin Falling Edge Detect Enable
    GPIO_INT_FALLING(gpio_pin, 1);
    
    // clear interrupt flag
    GPIO_INT_CLEAR(gpio_pin);
    
    spin_unlock_irqrestore(&lock, flags);
    
    return 0;
}

// Called when a process wants to read the dht11 "cat /dev/dht11"
static int read_dht11(struct inode *inode, struct file *file)
{
    char result[3];            //To say if the result is trustworthy or not
    int retry = 0;
    
    if (dht.control.Device_Open)
        return -EBUSY;
    
    try_module_get(THIS_MODULE);        //Increase use count
    
    dht.control.Device_Open++;
    
    // Take data low for min 18mS to start up DHT11
    //printk(KERN_INFO DHT11_DRIVER_NAME " Start setup (read_dht11)\n");
    
start_read:
    resetDht(&dht);
    GPIO_DIR_OUTPUT(gpio_pin);     // Set pin to output
    GPIO_CLEAR_PIN(gpio_pin);    // Set low
    mdelay(20);                    // DHT11 needs min 18mS to signal a startup
    GPIO_SET_PIN(gpio_pin);        // Take pin high
    udelay(40);                    // Stay high for a bit before swapping to read mode
    GPIO_DIR_INPUT(gpio_pin);     // Change to read
    
    //Start timer to time pulse length
    do_gettimeofday(&dht.control.lasttv);
    
    // Set up interrupts
    setup_interrupts();
    
    //Give the dht11 time to reply
    mdelay(10);
    
    //Check if the read results are valid. If not then try again!
    if(isDhtDataValid(&dht)) {
        sprintf(result, "OK");
    } else {
        retry++;
        sprintf(result, "BAD");
        if(retry == 5)
            goto return_result;        //We tried 5 times so bail out
        clear_interrupts();
        mdelay(1100);                //Can only read from sensor every 1 second so give it time to recover
        goto start_read;
    }
    
//Return the result in various different formats
return_result:
    sprintf(msg, "Temperature: %dC\nHumidity: %d%%\nResult:%s\n", dht.TemperatureInt, dht.HumidityInt, result);
    msg_Ptr = msg;
    
    return SUCCESS;
}

// Called when a process closes the device file.
static int close_dht11(struct inode *inode, struct file *file)
{
    // Decrement the usage count, or else once you opened the file, you'll never get get rid of the module.
    module_put(THIS_MODULE);
    dht.control.Device_Open--;
    
    clear_interrupts();
    //printk(KERN_INFO DHT11_DRIVER_NAME ": Device release (close_dht11)\n");
    
    return 0;
}

// Clear the GPIO edge detect interrupts
static void clear_interrupts(void)
{
    unsigned long flags;
    
    spin_lock_irqsave(&lock, flags);
    
    // GPREN0 GPIO Pin Rising Edge Detect Disable
    GPIO_INT_RISING(gpio_pin, 0);
    
    // GPFEN0 GPIO Pin Falling Edge Detect Disable
    GPIO_INT_FALLING(gpio_pin, 0);
    
    spin_unlock_irqrestore(&lock, flags);
    
    free_irq(irq_any_gpio, GPIO_ANY_GPIO_DEVICE_DESC);
}

// Called when a process, which already opened the dev file, attempts to read from it.
static ssize_t device_read(struct file *filp,    // see include/linux/fs.h
                           char *buffer,    // buffer to fill with data
                           size_t length,    // length of the buffer
                           loff_t * offset)
{
    // Number of bytes actually written to the buffer
    int bytes_read = 0;
    
    // If we're at the end of the message, return 0 signifying end of file
    if (*msg_Ptr == 0)
        return 0;
    
    // Actually put the data into the buffer
    while (length && *msg_Ptr) {
        
        // The buffer is in the user data segment, not the kernel  segment so "*" assignment won't work.  We have to use
        // put_user which copies data from the kernel data segment to the user data segment.
        put_user(*(msg_Ptr++), buffer++);
        
        length--;
        bytes_read++;
    }
    
    // Return the number of bytes put into the buffer
    return bytes_read;
}

module_init(dht11_init_module);
module_exit(dht11_exit_module);

MODULE_DESCRIPTION("DHT11 temperature/humidity sendor driver for Raspberry Pi GPIO.");
MODULE_AUTHOR("Nigel Morton");
MODULE_LICENSE("GPL");

// Command line paramaters for gpio pin and driver major number
module_param(format, int, S_IRUGO);
MODULE_PARM_DESC(format, "Format of output");

module_param(gpio_pin, int, S_IRUGO);
MODULE_PARM_DESC(gpio_pin, "GPIO pin to use");

module_param(driverno, int, S_IRUGO);
MODULE_PARM_DESC(driverno, "Driver handler major value");

