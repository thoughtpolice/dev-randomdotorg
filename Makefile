randomdotorg-objs := rdo.o # http_parser.o
obj-m             := randomdotorg.o
KDIR              := /lib/modules/$(shell uname -r)/build
PWD               := $(shell pwd)

EXTRA_CFLAGS      += -Wall

ifeq ($(KERNELRELEASE),)
	include config.mk
endif

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -f *.mod.c *.ko *.o *~ *.symvers *.order .randomdotorg.*
	rm -rf .tmp_versions config.h config.mk
