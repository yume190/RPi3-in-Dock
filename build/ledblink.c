#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <mach/platform.h>
#include <linux/timer.h>
#include <linux/device.h>
#include <linux/err.h>

MODULE_LICENSE("GPL");

//static struct bin_attribute edid_attr = {
//    .attr.name = "edid",
//    .attr.mode = 0444,
//    @@ -359,6 +391,27 @@ static struct bin_attribute edid_attr = {
//        .read = edid_show,
//    };
//
//struct attribute_group {
//    84         const char              *name;
//    85         umode_t                 (*is_visible)(struct kobject *,
//                                                     86                                               struct attribute *, int);
//    87         umode_t                 (*is_bin_visible)(struct kobject *,
//                                                         88                                                   struct bin_attribute *, int);
//    89         struct attribute        **attrs;
//    90         struct bin_attribute    **bin_attrs;
//    91 };
//
//static struct attribute_group dgrp_global_settings_attribute_group = {
//    .attrs = dgrp_sysfs_global_settings_entries,
//};
//
//
//static const struct attribute_group *dgrp_global_settings_attribute_groups[] = {
//    &dgrp_global_settings_attribute_group,
//    NULL
//};

struct GpioRegisters
{
    uint32_t GPFSEL[6];
    uint32_t Reserved1;
    uint32_t GPSET[2];
    uint32_t Reserved2;
    uint32_t GPCLR[2];
};

struct GpioRegisters *s_pGpioRegisters;

static void SetGPIOFunction(int GPIO, int functionCode)
{
    int registerIndex = GPIO / 10;
    int bit = (GPIO % 10) * 3;
    
    unsigned oldValue = s_pGpioRegisters->GPFSEL[registerIndex];
    unsigned mask = 0b111 << bit;
    printk("Changing function of GPIO%d from %x to %x\n", GPIO, (oldValue >> bit) & 0b111, functionCode);
    s_pGpioRegisters->GPFSEL[registerIndex] = (oldValue & ~mask) | ((functionCode << bit) & mask);
}

static void SetGPIOOutputValue(int GPIO, bool outputValue)
{
    if (outputValue)
        s_pGpioRegisters->GPSET[GPIO / 32] = (1 << (GPIO % 32));
    else
        s_pGpioRegisters->GPCLR[GPIO / 32] = (1 << (GPIO % 32));
}

static struct timer_list s_BlinkTimer;
static int s_BlinkPeriod = 2000;
static const int LedGpioPin = 23;

static void BlinkTimerHandler(unsigned long unused)
{
    static bool on = false;
    on = !on;
    SetGPIOOutputValue(LedGpioPin, on);
    mod_timer(&s_BlinkTimer, jiffies + msecs_to_jiffies(s_BlinkPeriod));
}

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
    
    s_BlinkPeriod = period_value;
    return count;
}

static DEVICE_ATTR(period, S_IWUSR | S_IWGRP | S_IWOTH, NULL, set_period_callback);

static struct class *s_pDeviceClass;
static struct device *s_pDeviceObject;

static int __init LedBlinkModule_init(void)
{
    int result;
    
    s_pGpioRegisters = (struct GpioRegisters *)__io_address(GPIO_BASE);
    SetGPIOFunction(LedGpioPin, 0b001);	//Configure the pin as output
    
    setup_timer(&s_BlinkTimer, BlinkTimerHandler, 0);
    result = mod_timer(&s_BlinkTimer, jiffies + msecs_to_jiffies(s_BlinkPeriod));
    BUG_ON(result < 0);
    
    s_pDeviceClass = class_create(THIS_MODULE, "LedBlink");
    BUG_ON(IS_ERR(s_pDeviceClass));
    
    s_pDeviceObject = device_create(s_pDeviceClass, NULL, 0, NULL, "LedBlink");
    BUG_ON(IS_ERR(s_pDeviceObject));
    
    result = device_create_file(s_pDeviceObject, &dev_attr_period);
    BUG_ON(result < 0);
    
    return 0;
}

static void __exit LedBlinkModule_exit(void)
{
    device_remove_file(s_pDeviceObject, &dev_attr_period);
    device_destroy(s_pDeviceClass, 0);
    class_destroy(s_pDeviceClass);
    
    SetGPIOFunction(LedGpioPin, 0);	//Configure the pin as input
    del_timer(&s_BlinkTimer);
}

module_init(LedBlinkModule_init);
module_exit(LedBlinkModule_exit);