randomdotorg-objs := rdo.o http_parser.o
obj-m             := randomdotorg.o
KDIR              := /lib/modules/$(shell uname -r)/build
PWD               := $(shell pwd)

EXTRA_CFLAGS      += -Wall

ifeq ($(KERNELRELEASE),)
	include config.mk
endif

default: test
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
test: ioctl.h test.c
	@echo "Building test program..."
	@echo "  CC test.o"
	@echo "  LD test"
	@gcc -Wall -o test test.c

clean:
	rm -f *.mod.c *.ko *.o *~ *.symvers *.order .randomdotorg.* .*.cmd test
	rm -rf .tmp_versions 
distclean: clean
	rm -f config.h config.mk
