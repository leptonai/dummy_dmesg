obj-m += dummy_dmesg.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install:
	sudo insmod dummy_dmesg.ko

uninstall:
	sudo rmmod dummy_dmesg