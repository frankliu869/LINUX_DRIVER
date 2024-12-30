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
#include <linux/spi/spi.h>


#define DEVICE_NAME "testSPIDevice"
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

static struct charDevice test_spi_device;

static int test_spi_device_write_data(struct spi_device *spi, u8 reg, void *buf, int len){
	struct spi_message m;
	struct spi_transfer t;
	u8 *txdata;
	int ret = 0;

	txdata = kzalloc(sizeof(u8)+len, GFP_KERNEL);
	if(!txdata){
		return -ENOMEM;
	}

	txdata[0] = reg & ~0x80;
	memcpy(txdata+1, buf, len);
	t.tx_buf = txdata;
	t.len = len+1;

	spi_message_init(&m);
	spi_message_add_tail(&t, &m);

	ret = spi_sync(spi, &m);
	if(ret){
		printk(KERN_INFO "spi write data failed\n");
	}

	kfree(txdata);

	return ret;
}


static int test_spi_device_read_data(struct spi_device *spi, u8 reg, void *buf, int len){

	int ret = 0;

	ret = spi_write(spi, &reg, 1);
	if(ret){
		printk(KERN_INFO "spi read data failed\n");
		return -1;
	}

	ret = spi_read(spi, buf, len);
	if(ret){
		printk(KERN_INFO "spi read data failed\n");
		return -1;
	}

	return 0;
}

static int test_spi_device_open(struct inode *inode, struct file *file){
	printk("test_device_open\n");
	file->private_data = &test_spi_device;
	return 0;
}

static ssize_t test_spi_device_read(struct file *file, char __user *buf, size_t count, loff_t *offset){
	printk("test_device_read\n");
	return 0;
}

static int test_spi_device_release(struct inode *inode, struct file *file){
	printk("test_device_release\n");
	return 0;
}

static const struct file_operations test_spi_device_ops = {
	.owner = THIS_MODULE,
	.open = test_spi_device_open,
	.read = test_spi_device_read,
	.release = test_spi_device_release,
};

int test_spi_device_probe(struct spi_device *spi){
    printk("test_device_probe\n");

	int ret = 0;

	if(test_spi_device.major){
		test_spi_device.devId = MKDEV(test_spi_device.major, 0);
		register_chrdev_region(test_spi_device.devId, NUM_DEVICES, DEVICE_NAME);
	}else{
		alloc_chrdev_region(&test_spi_device.devId, 0, NUM_DEVICES, DEVICE_NAME);
		test_spi_device.major = MAJOR(test_spi_device.devId);
		test_spi_device.minor = MINOR(test_spi_device.devId);
	}

	test_spi_device.testCdev.owner = THIS_MODULE;
	cdev_init(&test_spi_device.testCdev, &test_spi_device_ops);
	if(cdev_add(&test_spi_device.testCdev, test_spi_device.devId, 1) < 0){
		printk(KERN_INFO "cdev add failed\n");
		goto CDEV_ERR;
	}

	test_spi_device.class = class_create(DEVICE_NAME);
	if(IS_ERR(test_spi_device.class)){
		printk(KERN_INFO "class create failed\n");
		goto CLASS_ERR;
	}

	test_spi_device.device = device_create(test_spi_device.class, NULL, test_spi_device.devId, NULL, DEVICE_NAME);
	if(IS_ERR(test_spi_device.device)){
		printk(KERN_INFO "device create failed\n");
		goto DEV_ERR;
	}

	spi->mode = SPI_MODE_0;
	if(spi_setup(spi) < 0){
		printk(KERN_INFO "spi setup failed\n");
		goto DEV_ERR;
	}
	test_spi_device.private_data = spi;

	//device init function ...

	return 0;

DEV_ERR:
    device_destroy(test_spi_device.class, test_spi_device.devId);
CLASS_ERR:
    class_destroy(test_spi_device.class);
CDEV_ERR:
    cdev_del(&test_spi_device.testCdev);
    unregister_chrdev_region(test_spi_device.devId, NUM_DEVICES);

    return -1;

}

void test_spi_device_remove(struct spi_device *spi){

    device_destroy(test_spi_device.class, test_spi_device.devId);

    class_destroy(test_spi_device.class);

    cdev_del(&test_spi_device.testCdev);
    unregister_chrdev_region(test_spi_device.devId, NUM_DEVICES);
}


/* non device tree match table */
static const struct spi_device_id test_device_id[] = {
	{"test", 0},  
	{ /* Sentinel */ }
};

/* device tree match table */
static const struct of_device_id test_device_of_match[] = {
	{ .compatible = "test" },
	{ /* Sentinel */ }
};

/* i2c driver struct */	
static struct spi_driver test_spi_driver = {
	.probe = test_spi_device_probe,
	.remove = test_spi_device_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "test_device",
		   	.of_match_table = test_device_of_match, 
		   },
	.id_table = test_device_id,
};

static int __init test_spi_device_init(void){
	int ret = 0;
	ret = spi_register_driver(&test_spi_driver);
	if(ret < 0){
		printk(KERN_INFO "spi driver add failed\n");
	}

	return ret;
}

static void __exit test_spi_device_exit(void){
	spi_unregister_driver(&test_spi_driver);
	printk(KERN_INFO "spi driver removed\n");
}


module_init(test_spi_device_init);
module_exit(test_spi_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Siyuan Liu");
MODULE_DESCRIPTION("A Simple i2c Device Driver");