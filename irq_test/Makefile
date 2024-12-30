obj-m += irq_test.o

KDIR := /home/oran-5g/rpi/modules/lib/modules/6.6.41-v7+/build/
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(PWD) clean