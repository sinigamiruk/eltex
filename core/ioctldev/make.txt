#$KDIR is absolute path to directory source of linux

obj-m += ioctl_dev.o

KDIR ?= /lib/modules/$(shell uname -r)/build
	
all: main
		make -C $(KDIR) M=$(PWD) modules

main: main.c
		gcc -o main main.c

clean:
		make -C $(KDIR) M=$(PWD) clean 
		rm main

