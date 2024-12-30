#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
//#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/atomic.h>
#include <linux/fs.h>


#define DEVICE_NAME "testCharDev"
#define NUM_DEVICES 1

#define KEY_GPIO (int)517 // GPIO5
#define KEY_IRQ  (int)22

typedef struct keyDesc {
    int gpio;
    int irqnum;
    unsigned char value;
    char name[10];
    irqreturn_t (*handler)(int, void *);
    struct timer_list timer;
} keyDesc;

typedef struct charDevice {
    dev_t devId;
    int major;
    int minor;
    struct cdev testCdev;
    struct class *class;
    struct device *device;
    int data;
    int lockStatus;
    atomic_t keyGpio5_resource;
    
    struct keyDesc keyGpio5Desc;
} charDevice;

static struct charDevice testDevice;

static irqreturn_t keyGpio5Handler(int irq, void *dev_id){
    struct charDevice *dev = (struct charDevice *)dev_id;
    mod_timer(&testDevice.keyGpio5Desc.timer, jiffies + msecs_to_jiffies(10));

    return IRQ_RETVAL(IRQ_HANDLED);
}

void timerCallback(struct timer_list *timer){
    printk(KERN_INFO "Timer callback\n");

    if(atomic_read(&testDevice.keyGpio5_resource) == 1){
        atomic_set(&testDevice.keyGpio5_resource, 0);
        if(gpio_get_value(KEY_GPIO) == 0){
            printk(KERN_INFO "Key pressed\n");
        }
        atomic_set(&testDevice.keyGpio5_resource, 1);
    }else{
        printk(KERN_INFO "Resource busy\n");
    }
}

static int keyGpio5_init(void){
    int ret = 0;

    testDevice.keyGpio5Desc.gpio = KEY_GPIO;
    memset(testDevice.keyGpio5Desc.name, 0, sizeof(testDevice.keyGpio5Desc.name));
    sprintf(testDevice.keyGpio5Desc.name, "keyGpio5");

    ret = gpio_request(KEY_GPIO, "keyGpio5_irq");
    if(ret){
        printk(KERN_ERR "Failed to request GPIO\n");
        printk(KERN_ERR "Error code: %d\n", ret);
        return -1;
    }

    if(gpio_direction_input(KEY_GPIO)){
        printk(KERN_ERR "Failed to set GPIO direction\n");
        return -1;
    }

    testDevice.keyGpio5Desc.irqnum = gpio_to_irq(testDevice.keyGpio5Desc.gpio);
    if(testDevice.keyGpio5Desc.irqnum < 0){
        printk(KERN_ERR "Failed to get IRQ number\n");
    }else{
        printk(KERN_ERR "IRQ number: %d\r\n", testDevice.keyGpio5Desc.irqnum);
    }
    
    testDevice.keyGpio5Desc.handler = keyGpio5Handler;
    ret = request_irq(testDevice.keyGpio5Desc.irqnum, testDevice.keyGpio5Desc.handler, 
                      IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, testDevice.keyGpio5Desc.name, &testDevice);
    if(ret < 0){
        printk(KERN_ERR "Failed to request IRQ\n");
        printk(KERN_ERR "Error code: %d\n", ret);
        return -1;
    }

    testDevice.keyGpio5Desc.value = 0x01;
    

    timer_setup(&testDevice.keyGpio5Desc.timer, timerCallback, 0);
    atomic_set(&testDevice.keyGpio5_resource, 1);

    return 0;
}

static void keyGpio5_exit(void){
    del_timer_sync(&testDevice.keyGpio5Desc.timer);
    free_irq(testDevice.keyGpio5Desc.irqnum, &testDevice);
    gpio_free(testDevice.keyGpio5Desc.gpio);
}


static ssize_t templateCharDev_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
     return 0;
}

static ssize_t templateCharDev_read(struct file *file, char __user *buffer, size_t len, loff_t *offset) {
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



static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = templateCharDev_open,
    .release = templateCharDev_release,
    .read = templateCharDev_read,
    .write = templateCharDev_write,
};

static int __init templateCharDev_init(void) {

    int errorCode = 0;

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
        goto CDEV_ERR;
    }

    testDevice.class = class_create(DEVICE_NAME);
    if(IS_ERR(testDevice.class)){
        printk(KERN_ERR "testCharDev: Failed to create class\n");
        goto CLASS_ERR;
    }

    testDevice.device = device_create(testDevice.class, NULL, testDevice.devId, NULL, DEVICE_NAME);
    if(IS_ERR(testDevice.device)){
        printk(KERN_ERR "testCharDev: Failed to create device\n");
        goto DEV_ERR;
    }
    
    if(keyGpio5_init() < 0){
        printk(KERN_ERR "Failed to initialize keyGpio5\n");
        goto GPIO_ERR;
    }

    printk(KERN_INFO "testCharDev: Device registered\n");
    return 0;

GPIO_ERR:
    keyGpio5_exit();
DEV_ERR:
    device_destroy(testDevice.class, testDevice.devId);
CLASS_ERR:
    class_destroy(testDevice.class);
CDEV_ERR:
    cdev_del(&testDevice.testCdev);
    unregister_chrdev_region(testDevice.devId, NUM_DEVICES);

    return -1;
}


static void __exit templateCharDev_exit(void) {
    keyGpio5_exit();
    device_destroy(testDevice.class, testDevice.devId);
    class_destroy(testDevice.class);    
    cdev_del(&testDevice.testCdev);
    unregister_chrdev_region(testDevice.devId, NUM_DEVICES);
    printk(KERN_INFO "testCharDev: Device unregistered\n");
}

module_init(templateCharDev_init);
module_exit(templateCharDev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siyuan Liu");
MODULE_DESCRIPTION("A Simple Character Device Driver");
