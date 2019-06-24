obj-m := nunchuk.o
KDIR := $(HOME)/linux-kernel/linux-rpi

default:
	make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
