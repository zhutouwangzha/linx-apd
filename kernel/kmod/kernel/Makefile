MODULE_NAME := sysmon

CURD = $(patsubst %/,%, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
SRCS := $(shell cd $(CURD) && ls -f ./*.c 2>/dev/null | xargs)
OBJS := $(patsubst %.c,%.o,$(SRCS))

obj-m := $(MODULE_NAME).o
$(MODULE_NAME)-y ?= $(OBJS)

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD	= $(shell pwd)

ccflags-y += -I$(CURD)/../include
ccflags-y += -I$(CURD)/../include/kernel 

.PHONY: all clean

all:
	@echo $(CURD)
	@echo $(PWD)
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
