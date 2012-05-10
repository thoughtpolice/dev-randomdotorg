randomdotorg-objs := rdo.o # http_parser.o
obj-m             := randomdotorg.o
KDIR              := /lib/modules/$(shell uname -r)/build
PWD               := $(shell pwd)

EXTRA_CFLAGS      += -Wall

# A stupid - but working - hack!
SUBDIRS           ?= $(PWD)
include $(SUBDIRS)/config.mk

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -f *.mod.c *.ko *.o *~ *.symvers *.order .randomdotorg.*
	rm -rf .tmp_versions config.h config.mk
