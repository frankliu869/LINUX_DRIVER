#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>

#define LED_MAGIC 'k'
#define LED_INIT _IO(LED_MAGIC, 0)
#define LED_NO   _IOW(LED_MAGIC, 1, int)
#define LED_OFF  _IOW(LED_MAGIC, 2, int) 

#define DEVICE_NAME "testCharDev"
#define NUM_DEVICES 1

typedef struct timerDevice {
    dev_t devId;
    int major;
    int minor;
    struct cdev testCdev;
    struct class *class;
    struct device *device;
    int data;
    int lockStatus;
    struct mutex ledLock;
    struct timer_list timer;
} timerDevice;

static struct timerDevice testDevice;


static ssize_t templateCharDev_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
     return 0;
}

static ssize_t templateCharDev_read(struct file *file, char __user *buffer, size_t len, loff_t *offset) {
    return 0;
}

long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    int temp;

    mutex_lock(&testDevice.ledLock);

    switch (cmd)
    {
    case LED_INIT:
        printk(KERN_INFO "LED_INIT\n");
        // gpio_request(17, "LED");
        // gpio_direction_output(17, 1);
        break;

    case LED_NO:
        printk(KERN_INFO "LED_NO\n");

        copy_from_user(&temp, (int *)arg, sizeof(int));
        printk(KERN_INFO "arg = %ld\n", temp);
        // gpio_set_value(17, 0);
        break;

    case LED_OFF:
        printk(KERN_INFO "LED_OFF\n");

        copy_from_user(&temp, (int *)arg, sizeof(int));
        printk(KERN_INFO "arg = %ld\n", temp);
        // gpio_set_value(17, 1);
        break;

    default:
        mutex_unlock(&testDevice.ledLock);
        return -EINVAL;
    }

    mutex_unlock(&testDevice.ledLock);

    return 0;
}

static int templateCharDev_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "testCharDev: Device opened\n");
    file->private_data = &testDevice;

    return 0;
}

static int templateCharDev_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "testCharDev: Device closed\n");
    return 0;
}

void timerCallback(struct timer_list *timer){
    printk(KERN_INFO "Timer expired and Restart !\n");
    mod_timer(&testDevice.timer, jiffies + msecs_to_jiffies(1000));
    // gpio_set_value(17, 1);
}



static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = templateCharDev_open,
    .release = templateCharDev_release,
    .read = templateCharDev_read,
    .write = templateCharDev_write,
    .unlocked_ioctl = led_ioctl,
};

static int __init templateCharDev_init(void) {

    int errorCode = 0;

    mutex_init(&testDevice.ledLock);
    testDevice.lockStatus = 0;

    if(testDevice.minor){
        testDevice.devId = MKDEV(testDevice.major, 0);
        register_chrdev_region(testDevice.devId, NUM_DEVICES, DEVICE_NAME);
    }else{
        alloc_chrdev_region(&testDevice.devId, 0, NUM_DEVICES, DEVICE_NAME);
        testDevice.major = MAJOR(testDevice.devId);
        testDevice.minor = MINOR(testDevice.devId); 
    }

    testDevice.testCdev.owner = THIS_MODULE;
    cdev_init(&testDevice.testCdev, &fops);
    errorCode = cdev_add(&testDevice.testCdev, testDevice.devId, NUM_DEVICES);
    if(errorCode < 0){
        printk(KERN_ERR "testCharDev: Failed to add device\n");
        return errorCode;
    }

    testDevice.class = class_create(DEVICE_NAME);
    if(IS_ERR(testDevice.class)){
        printk(KERN_ERR "testCharDev: Failed to create class\n");
        return PTR_ERR(testDevice.class);
    }

    testDevice.device = device_create(testDevice.class, NULL, testDevice.devId, NULL, DEVICE_NAME);
    if(IS_ERR(testDevice.device)){
        printk(KERN_ERR "testCharDev: Failed to create device\n");
        return PTR_ERR(testDevice.device);
    }

    timer_setup(&testDevice.timer, timerCallback, 0);
    mod_timer(&testDevice.timer, jiffies + msecs_to_jiffies(1000));
    
    printk(KERN_INFO "testCharDev: Device registered\n");
    return 0;
}


static void __exit templateCharDev_exit(void) {
    del_timer(&testDevice.timer);

    device_destroy(testDevice.class, testDevice.devId);
    class_destroy(testDevice.class);    
    cdev_del(&testDevice.testCdev);

    printk(KERN_INFO "testCharDev: Device unregistered\n");
}

module_init(templateCharDev_init);
module_exit(templateCharDev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siyuan Liu");
MODULE_DESCRIPTION("A Simple Character Device Driver");
