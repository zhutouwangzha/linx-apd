# 多线程事件丰富化 Makefile

CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -std=c99 -D_GNU_SOURCE
LDFLAGS = -lpthread

# 包含路径
INCLUDES = -I./linx_event_rich/include \
           -I./linx_hash_map/include \
           -I./linx_event_get/include \
           -I./linx_log/include \
           -I./linx_process_cache/include \
           -I./linx_machine_status/include \
           -I./linx_fd_info/include

# 源文件
LINX_EVENT_RICH_SRCS = linx_event_rich/linx_event_rich.c \
                       linx_event_rich/linx_thread_context.c \
                       linx_event_rich/linx_event_rich_mt.c

LINX_HASH_MAP_SRCS = linx_hash_map/linx_hash_map.c

# 目标文件
LINX_EVENT_RICH_OBJS = $(LINX_EVENT_RICH_SRCS:.c=.o)
LINX_HASH_MAP_OBJS = $(LINX_HASH_MAP_SRCS:.c=.o)

# 库文件
LIBLINX_EVENT_RICH = liblinx_event_rich.a
LIBLINX_HASH_MAP = liblinx_hash_map.a

# 示例程序
EXAMPLE_SRCS = examples/multithread_example.c
EXAMPLE_TARGET = multithread_example

.PHONY: all clean libs example

all: libs example

# 构建库文件
libs: $(LIBLINX_EVENT_RICH) $(LIBLINX_HASH_MAP)

$(LIBLINX_EVENT_RICH): $(LINX_EVENT_RICH_OBJS)
	ar rcs $@ $^

$(LIBLINX_HASH_MAP): $(LINX_HASH_MAP_OBJS)
	ar rcs $@ $^

# 构建示例程序
example: $(EXAMPLE_TARGET)

$(EXAMPLE_TARGET): $(EXAMPLE_SRCS) $(LIBLINX_EVENT_RICH) $(LIBLINX_HASH_MAP)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

# 编译规则
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 清理
clean:
	rm -f $(LINX_EVENT_RICH_OBJS) $(LINX_HASH_MAP_OBJS)
	rm -f $(LIBLINX_EVENT_RICH) $(LIBLINX_HASH_MAP)
	rm -f $(EXAMPLE_TARGET)

# 安装头文件（可选）
install-headers:
	mkdir -p /usr/local/include/linx
	cp linx_event_rich/include/*.h /usr/local/include/linx/
	cp linx_hash_map/include/*.h /usr/local/include/linx/

# 调试信息
debug:
	@echo "LINX_EVENT_RICH_SRCS: $(LINX_EVENT_RICH_SRCS)"
	@echo "LINX_EVENT_RICH_OBJS: $(LINX_EVENT_RICH_OBJS)"
	@echo "LINX_HASH_MAP_SRCS: $(LINX_HASH_MAP_SRCS)"
	@echo "LINX_HASH_MAP_OBJS: $(LINX_HASH_MAP_OBJS)"
	@echo "INCLUDES: $(INCLUDES)"