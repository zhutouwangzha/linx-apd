MODULE_NAME := linxapd
BUILD_PATH 	:= $(BUILD_DIR)/kmod

CURD = $(patsubst %/,%, $(dir $(abspath $(lastword $(MAKEFILE_LIST)))))
TOP_DIR = $(CURD)/../..
EVENT_TABLE_DIR += $(TOP_DIR)/userspace/linx_event_table/linx_event_table.c \
				   $(TOP_DIR)/userspace/linx_event_table/linx_name_value.c

SRCS := $(shell cd $(CURD) && ls -f ./src/*.c 2>/dev/null | xargs)

SRCS += $(shell realpath --relative-to=$(CURD) $(EVENT_TABLE_DIR))

OBJS := $(patsubst %.c,%.o,$(SRCS))

obj-m := $(MODULE_NAME).o
$(MODULE_NAME)-y ?= $(OBJS)

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD	= $(shell pwd)

ccflags-y += -I$(CURD)/include
ccflags-y += -I$(CURD)/../../include/ 

.PHONY: all clean

all:
	@echo $(CURD)
	@echo $(SRCS)
	@echo $(TOP_DIR)
	make -C $(KDIR) M=$(PWD) modules
	@mkdir -p $(BUILD_PATH)/
	cp -f $(obj-m:.o=.ko) $(BUILD_PATH)/

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm -rf $(BUILD_PATH)/
