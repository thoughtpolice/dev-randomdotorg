obj-m := randomdotorg.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -f *.mod.c *.ko *.o *~ *.symvers *.order .randomdotorg.*
	rm -rf .tmp_versions
