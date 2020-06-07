obj-m += input_test_driver.o
 
PWD += $(shell pwd)

all:
	make -C /home/v4bel/kernel/Dev/defenit_ctf/linux-4.15 M=$(PWD) modules
 
clean:
	make -C /home/v4bel/kernel/Dev/defenit_ctf/linux-4.15 M=$(PWD) clean
