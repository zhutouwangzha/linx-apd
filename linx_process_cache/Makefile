# Makefile for linx_process_cache module

CC = gcc
CLANG = clang
CFLAGS = -Wall -Wextra -std=gnu99 -O2 -g
LDFLAGS = -pthread
BPF_CFLAGS = -O2 -g -target bpf -c

# 依赖库
LIBS = -lbpf -lelf -lz

# 目标文件
TARGETS = example_usage process_monitor.bpf.o

# 头文件路径
INCLUDES = -I./include -I../userspace/linx_thread/include

# 源文件
SRCS = linx_process_cache_enhanced.c linx_process_cache_ebpf.c example_usage.c
OBJS = $(SRCS:.c=.o)

# BPF程序
BPF_PROG = process_monitor.bpf.c
BPF_OBJ = process_monitor.bpf.o

.PHONY: all clean install

all: $(TARGETS)

# 编译eBPF程序
$(BPF_OBJ): $(BPF_PROG)
	$(CLANG) $(BPF_CFLAGS) $(INCLUDES) -o $@ $<

# 编译主程序
example_usage: $(OBJS) $(BPF_OBJ)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(filter %.o,$^) $(LDFLAGS) $(LIBS)

# 编译对象文件
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# 检查eBPF程序
check-bpf: $(BPF_OBJ)
	@echo "检查eBPF程序..."
	@llvm-objdump -h $(BPF_OBJ) || echo "需要安装llvm工具"

# 安装依赖（Ubuntu/Debian）
install-deps:
	@echo "安装编译依赖..."
	sudo apt-get update
	sudo apt-get install -y \
		build-essential \
		clang \
		llvm \
		libbpf-dev \
		libelf-dev \
		zlib1g-dev \
		linux-headers-$(shell uname -r)

# 测试运行
test: example_usage
	@echo "运行测试（需要root权限用于eBPF）..."
	sudo ./example_usage --help

# 测试无eBPF模式
test-no-ebpf: example_usage
	@echo "运行无eBPF测试..."
	./example_usage --no-ebpf --fast-scan

# 清理
clean:
	rm -f $(OBJS) $(TARGETS)

# 显示帮助
help:
	@echo "可用目标:"
	@echo "  all           - 编译所有目标"
	@echo "  example_usage - 编译示例程序"
	@echo "  check-bpf     - 检查eBPF程序"
	@echo "  install-deps  - 安装编译依赖"
	@echo "  test          - 运行测试（需要root权限）"
	@echo "  test-no-ebpf  - 运行无eBPF测试"
	@echo "  clean         - 清理编译文件"
	@echo "  help          - 显示此帮助"

# 依赖关系
linx_process_cache_enhanced.o: linx_process_cache_enhanced.c include/linx_process_cache.h include/linx_process_cache_ebpf.h
linx_process_cache_ebpf.o: linx_process_cache_ebpf.c include/linx_process_cache_ebpf.h include/linx_process_cache.h
example_usage.o: example_usage.c include/linx_process_cache.h include/linx_process_cache_ebpf.h