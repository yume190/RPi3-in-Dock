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
#include <mach/hardware.h>
#include "dht.h"

// module parameters
static int sense = 0;
static struct timeval lasttv = {0, 0};

static spinlock_t lock;

// Forward declarations
static int read_dht11(struct inode *, struct file *);
static int close_dht11(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static void clear_interrupts(void);

// Global variables are declared as static, so are global within the file.
static int Device_Open = 0;                // Is device open?  Used to prevent multiple access to device
static char msg[BUF_LEN];                // The msg the device will give when asked
static char *msg_Ptr;
static spinlock_t lock;
static unsigned int bitcount=0;
static unsigned int bytecount=0;
static unsigned int started=0;            //Indicate if we have started a read or not
static unsigned char dht[5];            // For result bytes

// IRQ handler - where the timing takes place
static irqreturn_t irq_handler(int i, void *blah, struct pt_regs *regs)
{
    struct timeval tv;
    long deltv;
    int data = 0;
    int signal;

    // use the GPIO signal level
    signal = GPIO_READ_PIN(gpio_pin);

    /* reset interrupt */
    GPIO_INT_CLEAR(gpio_pin);

    if (sense != -1) {
        // get current time
        do_gettimeofday(&tv);

        // get time since last interrupt in microseconds
        deltv = tv.tv_sec-lasttv.tv_sec;

        data = (int) (deltv*1000000 + (tv.tv_usec - lasttv.tv_usec));
        lasttv = tv;    //Save last interrupt time

        if((signal == 1)&(data > 40))
            {
            started = 1;
            return IRQ_HANDLED;
            }

        if((signal == 0)&(started==1))
            {
            if(data > 80)
                return IRQ_HANDLED;                                        //Start/spurious? signal
            if(data < 15)
                return IRQ_HANDLED;                                        //Spurious signal?
            if (data > 60)//55
                dht[bytecount] = dht[bytecount] | (0x80 >> bitcount);    //Add a 1 to the data byte

            //Uncomment to log bits and durations - may affect performance and not be accurate!
            //printk("B:%d, d:%d, dt:%d\n", bytecount, bitcount, data);
            bitcount++;
            if(bitcount == 8)
                {
                bitcount = 0;
                bytecount++;
                }
            //if(bytecount == 5)
            //    printk(KERN_INFO DHT11_DRIVER_NAME "Result: %d, %d, %d, %d, %d\n", dht[0], dht[1], dht[2], dht[3], dht[4]);
            }
        }
    return IRQ_HANDLED;
}

static int setup_interrupts(void)
{
    int result;
    unsigned long flags;

    result = request_irq(INTERRUPT_GPIO0, (irq_handler_t) irq_handler, 0, DHT11_DRIVER_NAME, (void*) gpio);

    switch (result) {
    case -EBUSY:
        printk(KERN_ERR DHT11_DRIVER_NAME ": IRQ %d is busy\n", INTERRUPT_GPIO0);
        return -EBUSY;
    case -EINVAL:
        printk(KERN_ERR DHT11_DRIVER_NAME ": Bad irq number or handler\n");
        return -EINVAL;
    default:
        printk(KERN_INFO DHT11_DRIVER_NAME    ": Interrupt %04x obtained\n", INTERRUPT_GPIO0);
        break;
    };

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



static ssize_t device_read(struct file *filp,    // see include/linux/fs.h
                           char *buffer,        // buffer to fill with data
                           size_t length,        // length of the buffer
                           loff_t * offset) {
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

static void init_communication() {
    // setup gpio
    //    GPIO_DIR_OUTPUT()
    //    GPIO_CLEAR_PIN()
    //    GPIO_SET_PIN()
    //    GPIO_DIR_INPUT()
    GPIO_DIR_OUTPUT(gpio_pin);
    GPIO_CLEAR_PIN(gpio_pin);
    mdelay(18);
    GPIO_SET_PIN(gpio_pin);
    udelay(40);
    GPIO_DIR_INPUT(gpio_pin);
}

static void setup_interrupt() {
    //setup interrupt
    //    request_irq()
    //    GPIO_INT_RISING()
    //    GPIO_INT_FALLING()
    //    GPIO_INT_CLEAR()
    unsigned long flags;
    request_irq(INTERRUPT_GPIO0, (irq_handler_t)irq_handler, 0, DHT11_DRIVER_NAME, (void*) gpio);
    GPIO_INT_RISING(gpio_pin, 1);
    GPIO_INT_FALLING(gpio_pin, 1);
    GPIO_INT_CLEAR(gpio_pin);
}

irqreturn_t irq_handler(...) {
    signal = GPIO_READ(gpio_pin);
    GPIO_INT_CLEAR(gpio_pin);
    if ((signal == 1) && (elapse > 50)) {
        started = 1;
        return IRQ_HANDLED;
    }
    if ((signal == 0) && (started == 1)) {
        if (elapse > 70) return IRQ_HANDLED;
        if (elapse < 26) return IRQ_HANDLED;
        if (elapse > 28)
        //return 1
    }
}

// Called when a process closes the device file.
static int device_release(struct inode *inode, struct file *file)
{
    // Decrement the usage count, or else once you opened the file, you'll never get get rid of the module.
    module_put(THIS_MODULE);
    Device_Open--;

    clear_interrupts();
    //printk(KERN_INFO DHT11_DRIVER_NAME ": Device release (close_dht11)\n");

    return 0;
}

void xxx() {
    unsigned int bitcnt = 0;
    unsigned int bytecnt = 0;
    unsigned char dht[5];
//    ...
    irq_handler() {
        if (elapse > 60)
            dht[bytecnt] = dht[bytecnt] | (0x80 >> bitcnt)
    }
}

static int device_open(struct inode *inode, struct file *file) {
    char result[3];            //To say if the result is trustworthy or not
        int retry = 0;

        if (Device_Open)
            return -EBUSY;

        try_module_get(THIS_MODULE);        //Increase use count

        Device_Open++;

        // Take data low for min 18mS to start up DHT11
        //printk(KERN_INFO DHT11_DRIVER_NAME " Start setup (read_dht11)\n");

    start_read:
        started = 0;
        bitcount = 0;
        bytecount = 0;
        dht[0] = 0;
        dht[1] = 0;
        dht[2] = 0;
        dht[3] = 0;
        dht[4] = 0;
        GPIO_DIR_OUTPUT(gpio_pin);     // Set pin to output
        GPIO_CLEAR_PIN(gpio_pin);    // Set low
        mdelay(20);                    // DHT11 needs min 18mS to signal a startup
        GPIO_SET_PIN(gpio_pin);        // Take pin high
        udelay(40);                    // Stay high for a bit before swapping to read mode
        GPIO_DIR_INPUT(gpio_pin);     // Change to read

        //Start timer to time pulse length
        do_gettimeofday(&lasttv);

        // Set up interrupts
        setup_interrupts();

        //Give the dht11 time to reply
        mdelay(10);

        //Check if the read results are valid. If not then try again!
        if((dht[0] + dht[1] + dht[2] + dht[3] == dht[4]) & (dht[4] > 0))
            sprintf(result, "OK");
        else
            {
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
        switch(format){
            case 0:
                sprintf(msg, "Values: %d, %d, %d, %d, %d, %s\n", dht[0], dht[1], dht[2], dht[3], dht[4], result);
                break;
            case 1:
                sprintf(msg, "%0X,%0X,%0X,%0X,%0X,%s\n", dht[0], dht[1], dht[2], dht[3], dht[4], result);
                break;
            case 2:
                sprintf(msg, "%02X%02X%02X%02X%02X%s\n", dht[0], dht[1], dht[2], dht[3], dht[4], result);
                break;
            case 3:
                sprintf(msg, "Temperature: %dC\nHumidity: %d%%\nResult:%s\n", dht[0], dht[2], result);
                break;

        }
        msg_Ptr = msg;

        return SUCCESS;
//    init_communication();
//    setup_interrupt();
}

static struct file_operations fops = {
    .open = device_open,
    .read = device_read,
    .release = device_release
};


