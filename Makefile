#makefile for list_module
obj-m:=monitor_nvidia0.o 
#changedev-objs:=chandev_list.o
KDIR:=/lib/modules/$(shell uname -r)/build
default:
	make -C $(KDIR) M=$(shell pwd) modules
	$(RM) *.mod.c *.mod.o *.o *.order *.symvers *.ko.unsigned *.cmd
clean:
	$(RM) *.ko *.mod.c *.mod.o *.o *.order *.symvers *.ko.unsigned *.cmd
