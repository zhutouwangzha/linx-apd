CLANG 		?= clang
LLVM_STRIP 	?= llvm-strip
LLVM_LINK	?= llvm-link
BPFTOOL 	?= bpftool
ARCH 		?= x86
BUILD_PATH 	:= $(BUILD_DIR)/ebpf

BPF_SRC 	= $(wildcard $(EBPF_DIR)/*.bpf.c $(EBPF_DIR)/tail_calls/*.bpf.c $(EBPF_DIR)/xdp/*.bpf.c)
BPF_OBJS 	= $(BPF_SRC:.c=.o)

MODE_NAME 		= linx_bpf
SKELETON_FILE 	= $(EBPF_DIR)/include/skel/$(MODE_NAME).skel.h
BPF_LINK		= $(BUILD_PATH)/$(MODE_NAME).ll
BPF_ELF			= $(BUILD_PATH)/$(MODE_NAME).bpf

INCLUDE 		= -I$(EBPF_DIR)/include -I$(TOPDIR)/include

DEBUG			= -g
WALL 			= -Wall
OPTIMIZE		= -O2

CLANG_FLAGS		= -target bpf -D__TARGET_ARCH_$(ARCH) $(DEBUG) $(WALL) $(OPTIMIZE) $(INCLUDE)

.PHONY: all bpf skeleton

all: bpf

bpf: $(BPF_ELF) skeleton

skeleton: $(SKELETON_FILE)

%.bpf.o: %.bpf.c
	@echo "[CLANG] $<"
	@$(CLANG) $(CLANG_FLAGS) -emit-llvm -S $< -o $@

$(BPF_LINK): $(BPF_OBJS)
	@mkdir -p $(BUILD_PATH)
	@echo "[LLVM_LINK] $@"
	@$(LLVM_LINK) $^ -S -o $@

$(BPF_ELF): $(BPF_LINK)
	@echo "[CLANG] $@"
	@$(CLANG) -target bpf -O2 -c $(BPF_LINK) -o $@
	@echo "[LLVM_STRIP] $@"
	@$(LLVM_STRIP) -g $@

$(SKELETON_FILE): $(BPF_ELF)
	@echo "[BPFTOOL] $@"
	@$(BPFTOOL) gen skeleton -p $< > $@ name $(MODE_NAME)

clean:
	@echo "[RM] $(BPF_OBJS) $(SKELETON_FILE)"
	@rm -rf $(BPF_OBJS) $(SKELETON_FILE)
