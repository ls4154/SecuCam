obj-m := led.o
#KDIR := /lib/modules/$(shell uname -r)/build
KDIR := /home/arch76/linux-kernel/linux-rpi

default:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
