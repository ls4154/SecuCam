obj-m := servo.o
KDIR := $(HOME)/linux-kernel/linux-rpi

default:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
