CC=x86_64-linux-gnu-gcc-12
obj-m+=hello.o

all:
	make CC=$(CC) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make CC=$(CC) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
