### Automatic Makefile generation by 'genmake' script        ####
### Copyright, Jerry Cooperstein, coop@linuxfoundation.org 2003-2023 ####
### License: GPLv2 ###

obj-m +=  process_times.o

process_times-y := file.o interval.o process.o proctime.o


export KROOT=/home/vui-chee/Desktop/repos/kernel-dojo/linux-6.18.6

allofit:  userprogs modules
modules:
	@$(MAKE) -C $(KROOT) M=$(shell pwd) modules
modules_install:
	@$(MAKE) -C $(KROOT) M=$(shell pwd) modules_install
kernel_clean:
	@$(MAKE) -C $(KROOT) M=$(shell pwd) clean

userprogs:
	@$(MAKE) \
		CFLAGS="-O2 -Wall -pthread" \
	 register

clean: kernel_clean
	rm -rf  register  Module.symvers modules.order
