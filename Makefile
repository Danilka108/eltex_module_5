# CC=x86_64-linux-gnu-gcc-12
obj-m+=lab1_module.o
obj-m+=lab2_module.o
obj-m+=lab3_module.o
obj-m+=lab4_module.o
obj-m+=lab5_module.o

.PHONY: all, clean, lab5_client

all: lab5_client
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

lab5_client: lab5_client.o
	$(CC) -c lab5_client.c
	$(CC) lab5_client.o -o lab5_client

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
