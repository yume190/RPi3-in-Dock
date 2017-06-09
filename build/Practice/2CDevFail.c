#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/types.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <mach/platform.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/err.h>

#define YUME_MAJOR 190 * 3
#define YUME_MINOR 0

static struct class *yume_class;
struct device *yume_device;

struct yume_dev {
    struct cdev cdev;
	char* str;
};

static ssize_t set_period_callback(struct device* dev,
								   struct device_attribute* attr,
								   const char* buf,
								   size_t count)
{
	long period_value = 0;
	if (kstrtol(buf, 10, &period_value) < 0)
		return -EINVAL;
	if (period_value < 10)	//Safety check
		return -EINVAL;
	
	return count;
}

static DEVICE_ATTR(period, 0644, NULL, set_period_callback);

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
	
	printk(KERN_INFO "File read %s\n", dev->str);
	if (copy_to_user(buffer, dev->str,1)) 
		return -1;
	return 1;
}

static ssize_t device_write(struct file *filp, const char __user *buffer, size_t length, loff_t *offset) {
	struct yume_dev *dev = filp->private_data;	
	
	if (copy_from_user(dev->str, buffer, 1))
		return -1;
	printk(KERN_INFO "File write %s\n", dev->str);
	return 1;
}

static int device_open(struct inode *inode, struct file *filp) {
	struct yume_dev *dev = container_of(inode->i_cdev, struct yume_dev, cdev);
	filp->private_data = dev;
	printk(KERN_INFO "File Open\n");
	return 0;
}

static int device_release(struct inode *inode, struct file *filp) {
	printk(KERN_INFO "File Release\n");
	return 0;
}

static void setup_yume_device(struct yume_dev *dev, int index) {
	int rc;
	int err, devno = MKDEV(YUME_MAJOR, YUME_MINOR + index);
	printk(KERN_INFO "MKDEV ing");
	
	cdev_init(&dev->cdev, &fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &fops;
	err = cdev_add(&dev->cdev, devno, 1);
	if (err < 0) {
		printk(KERN_INFO "cdev add fail");
	}
	printk(KERN_INFO "cdev add success!\n");
	printk(KERN_INFO "cdev %d:%d", MAJOR(dev->cdev.dev), MINOR(dev->cdev.dev));
	
	yume_device = device_create(
		yume_class, 
		NULL,
		dev->cdev.dev, 
		NULL,
	  	"%s", 
		"yume_device");
	rc = device_create_file(yume_device, &dev_attr_period);
//	BUG_ON(rc < 0);
//	rc = sysfs_create_link(
//		//&device->kobj,
//		dev->kobj,
//		&tcd->class_device->kobj,
//		tcd->mode_name
//	);
//	if (rc)
//		goto fail_with_class_device;
		
//	printk(KERN_INFO "cdev %s",	dev->cdev.kobj.name);
//	printk(KERN_INFO "cdev %s",	dev->cdev.kobj.kset->kobj.name);
}

static int __init yume_dev_init(void) {
	struct yume_dev yumeDev;
	yume_class = class_create(THIS_MODULE, "yume_class");
	my_cdev = cdev_alloc();
	yumeDev.cdev = *my_cdev;
	yumeDev.str = "";
		
	setup_yume_device(&yumeDev, 1);
	printk(KERN_INFO "Hello, world\n");
	return 0;
}

static void __exit yume_dev_exit(void) {
	printk(KERN_INFO "Goodbye, world\n");
	cdev_del(my_cdev);
	
	device_remove_file(yume_device, &dev_attr_period);
	device_destroy(yume_class, 0);
	class_destroy(yume_class);
//	class_destroy(yume_class);
	yume_class = NULL;
}

module_init(yume_dev_init);
module_exit(yume_dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yume");
MODULE_DESCRIPTION("Kernel Basic");        /* What does this module do */
MODULE_SUPPORTED_DEVICE("testdevice");