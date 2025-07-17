TOPDIR 		= $(shell pwd)
NAME 		= $(notdir $(CURDIR))
BUILD_PATH	= $(TOPDIR)/build
PLUGIN_OUT  = $(BUILD_PATH)/lib$(NAME).so

.PHONY: all bpf plugin test clean get_syscall

all: bpf plugin test

bpf: $(BUILD_PATH)
	make -C ebpf

plugin: bpf
	make -C plugin

test: $(BUILD_PATH)
	make -C test

$(BUILD_PATH):
	mkdir -p $@

clean:
	make -C ebpf clean
	make -C plugin clean
	make -C test clean
	rm -rf $(BUILD_PATH)

get_syscall:
	bash $(TOPDIR)/scripts/get_syscalls_macro.sh $(TOPDIR)/include/macro/linx_syscalls_macro.h

export TOPDIR
export BUILD_PATH
export PLUGIN_OUT
