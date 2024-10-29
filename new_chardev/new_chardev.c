#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "testCharDev"
#define NUM_DEVICES 1

typedef struct newCharDevice {
    dev_t devId;
    int major;
    int minor;
    struct cdev testCdev;
    struct class *class;
    struct device *device;
    int data;
} newCharDevice;

static struct newCharDevice testDevice;

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static ssize_t templateCharDev_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
     
}

static ssize_t templateCharDev_read(struct file *file, char __user *buffer, size_t len, loff_t *offset) {
    
}

static int __init templateCharDev_init(void) {

    int errorCode = 0;

    if(testDevice.minor){
        testDevice.devNum = MKDEV(newchrled.major, 0);
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

    testDevice.class = class_create(THIS_MODULE, DEVICE_NAME);
    if(IS_ERR(testDevice.class)){
        printk(KERN_ERR "testCharDev: Failed to create class\n");
        return PTR_ERR(testDevice.class);
    }

    testDevice.device = device_create(testDevice.class, NULL, testDevice.devId, NULL, DEVICE_NAME);
    if(IS_ERR(testDevice.device)){
        printk(KERN_ERR "testCharDev: Failed to create device\n");
        return PTR_ERR(testDevice.device);
    }
    
    printk(KERN_INFO "testCharDev: Device registered\n");
    return 0;
}


static void __exit templateCharDev_exit(void) {
    

    printk(KERN_INFO "mychardev: Device unregistered\n");
}

module_init(templateCharDev_init);
module_exit(templateCharDev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siyuan Liu");
MODULE_DESCRIPTION("A Simple Character Device Driver");
