ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
include Kbuild
else

KDIR ?= $(KERNEL_BUILD_DIR)

all:
	make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules

clean:
	make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- clean
	
endif
