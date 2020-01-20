MODULENAME = memqueue

obj-m += $(MODULENAME).o 
$(MODULENAME)-objs += memqueue_module.o
$(MODULENAME)-objs += mem_queue.o
$(MODULENAME)-objs += file_queue.o

module:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

# install:
# 	sudo insmod $(MODULENAME).ko

# uninstall:
# 	sudo rmmod $(MODULENAME)