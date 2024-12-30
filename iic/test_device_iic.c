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
#include <linux/i2c.h>


#define DEVICE_NAME "testI2CDevice"
#define NUM_DEVICES 1

typedef struct charDevice {
    dev_t devId;
    int major;
    int minor;
    struct cdev testCdev;
    struct class *class;
    struct device *device;
    void *private_data;
    int data;
} charDevice;

static struct charDevice test_i2c_device;

static int test_i2c_device_write_data(struct i2c_client *client, u8 reg, void *val, int len){
	u8 buf[2];
	struct i2c_msg msg[1];
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg *msgs = msg;
	int ret = 0;

	buf[0] = reg;
	buf[1] = *((u8 *)val);

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = buf;

	ret = i2c_transfer(adap, msgs, 1);
	if(ret < 0){
		printk(KERN_INFO "i2c write data failed\n");
		return -1;
	}

	return 0;
}


static u8* test_i2c_device_read_data(struct i2c_client *client, u8 reg, void *val, int len){
	u8 buf[2];
	struct i2c_msg msg[2];
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg *msgs = msg;
	int ret = 0;

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = &reg;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 2;
	msgs[1].buf = buf;

	ret = i2c_transfer(adap, msgs, 2);
	if(ret < 0){
		printk(KERN_INFO "i2c read data failed\n");
		return NULL;
	}

	return buf;
}

static int test_i2c_device_open(struct inode *inode, struct file *file){
	printk("test_device_open\n");
	file->private_data = &test_i2c_device;
	return 0;
}

static ssize_t test_i2c_device_read(struct file *file, char __user *buf, size_t count, loff_t *offset){
	printk("test_device_read\n");
	return 0;
}

static int test_i2c_device_release(struct inode *inode, struct file *file){
	printk("test_device_release\n");
	return 0;
}

static const struct file_operations test_i2c_device_ops = {
	.owner = THIS_MODULE,
	.open = test_i2c_device_open,
	.read = test_i2c_device_read,
	.release = test_i2c_device_release,
};

int test_i2c_device_probe(struct i2c_client *client){
    printk("test_device_probe\n");

	int ret = 0;

	if(test_i2c_device.major){
		test_i2c_device.devId = MKDEV(test_i2c_device.major, 0);
		register_chrdev_region(test_i2c_device.devId, NUM_DEVICES, DEVICE_NAME);
	}else{
		alloc_chrdev_region(&test_i2c_device.devId, 0, NUM_DEVICES, DEVICE_NAME);
		test_i2c_device.major = MAJOR(test_i2c_device.devId);
		test_i2c_device.minor = MINOR(test_i2c_device.devId);
	}

	test_i2c_device.testCdev.owner = THIS_MODULE;
	cdev_init(&test_i2c_device.testCdev, &test_i2c_device_ops);
	if(cdev_add(&test_i2c_device.testCdev, test_i2c_device.devId, 1) < 0){
		printk(KERN_INFO "cdev add failed\n");
		goto CDEV_ERR;
	}

	test_i2c_device.class = class_create(DEVICE_NAME);
	if(IS_ERR(test_i2c_device.class)){
		printk(KERN_INFO "class create failed\n");
		goto CLASS_ERR;
	}

	test_i2c_device.device = device_create(test_i2c_device.class, NULL, test_i2c_device.devId, NULL, DEVICE_NAME);
	if(IS_ERR(test_i2c_device.device)){
		printk(KERN_INFO "device create failed\n");
		goto DEV_ERR;
	}

	test_i2c_device.private_data = client;

	return 0;

DEV_ERR:
    device_destroy(test_i2c_device.class, test_i2c_device.devId);
CLASS_ERR:
    class_destroy(test_i2c_device.class);
CDEV_ERR:
    cdev_del(&test_i2c_device.testCdev);
    unregister_chrdev_region(test_i2c_device.devId, NUM_DEVICES);

    return -1;

}

void test_i2c_device_remove(struct i2c_client *client){

    device_destroy(test_i2c_device.class, test_i2c_device.devId);

    class_destroy(test_i2c_device.class);

    cdev_del(&test_i2c_device.testCdev);
    unregister_chrdev_region(test_i2c_device.devId, NUM_DEVICES);
}


/* non device tree match table */
static const struct i2c_device_id test_device_id[] = {
	{"test", 0},  
	{ /* Sentinel */ }
};

/* device tree match table */
static const struct of_device_id test_device_of_match[] = {
	{ .compatible = "test" },
	{ /* Sentinel */ }
};

/* i2c driver struct */	
static struct i2c_driver test_i2c_driver = {
	.probe = test_i2c_device_probe,
	.remove = test_i2c_device_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "test_device",
		   	.of_match_table = test_device_of_match, 
		   },
	.id_table = test_device_id,
};

static int __init test_i2c_device_init(void){
	int ret = 0;
	ret = i2c_add_driver(&test_i2c_driver);
	if(ret < 0){
		printk(KERN_INFO "i2c driver add failed\n");
	}

	return ret;
}

static void __exit test_i2c_device_exit(void){
	i2c_del_driver(&test_i2c_driver);
	printk(KERN_INFO "i2c driver removed\n");
}


module_init(test_i2c_device_init);
module_exit(test_i2c_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siyuan Liu");
MODULE_DESCRIPTION("A Simple i2c Device Driver");