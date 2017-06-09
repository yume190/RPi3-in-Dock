#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/fs.h>

#include <asm/uaccess.h>

static int yume_major = 190;
static short readPos = 0;
#define YUME_MINOR 0
#define DEVICE_NAME "YumeDevice"

struct yume_dev {
    struct cdev cdev;
	char str[100];
};

static struct cdev *my_cdev;
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release,
};

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) {
	struct yume_dev *dev = filp->private_data;
	
	short count = 0;
	while (length && (dev->str[readPos]!=0)) {
		put_user(dev->str[readPos], buffer++);
		count++;
		length--;
		readPos++;
	}
	return count;
}

static ssize_t device_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset) {
	struct yume_dev *dev = filp->private_data;	
	
	short ind = length - 1;
	short count = 0;
	memset(dev->str,0,100);
	readPos = 0;
	while(length > 0) {
		dev->str[count++] = buffer[ind--];
		length--;
	}
	return count;
}

static int device_open(struct inode *inode, struct file *filp) {
	struct yume_dev yumeDev = {
		.str = "yume",
	};
	my_cdev = cdev_alloc();
	yumeDev.cdev = *my_cdev;
	
//	struct yume_dev *dev = container_of(inode->i_cdev, struct yume_dev, cdev);
	filp->private_data = &yumeDev;
	printk(KERN_INFO "File Open\n");
	return 0;
}

static int device_release(struct inode *inode, struct file *filp) {
	printk(KERN_INFO "File Release\n");
	return 0;
}

static int __init yume_dev_init(void) {

	
	int rc = register_chrdev(yume_major, DEVICE_NAME, &fops);
	if (rc < 0) {
		yume_major = register_chrdev(0, DEVICE_NAME, &fops);
		if (yume_major < 0) {
			printk(KERN_ALERT "Registering char device failed with %d\n", yume_major);
			return yume_major;
		}
	}
	printk(KERN_INFO "I was assigned major number %d. To talk to\n", yume_major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, yume_major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");
	
	printk(KERN_INFO "Hello, world\n");
	return 0;
}

static void __exit yume_dev_exit(void) {
	printk(KERN_INFO "Goodbye, world\n");
	cdev_del(my_cdev);
	unregister_chrdev(yume_major, DEVICE_NAME);
}

module_init(yume_dev_init);
module_exit(yume_dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yume");
MODULE_DESCRIPTION("Kernel Basic");        /* What does this module do */
MODULE_SUPPORTED_DEVICE("testdevice");