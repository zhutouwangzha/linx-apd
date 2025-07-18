CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread -fPIC
INCLUDES = -Iinclude -I../linx_thread/include -I../linx_rule_engine/rule_engine_match/include -I../linx_hash_map/include
LDFLAGS = -pthread

# 源文件
SOURCES = linx_alert.c \
          stdout/linx_alert_stdout.c \
          file/linx_alert_file.c \
          http/linx_alert_http.c \
          syslog/linx_alert_syslog.c

# 目标文件
OBJECTS = $(SOURCES:.c=.o)

# 静态库目标
STATIC_LIB = liblinx_alert.a

# 动态库目标
SHARED_LIB = liblinx_alert.so

# 示例程序
EXAMPLE = example/linx_alert_example
EXAMPLE_SRC = example/linx_alert_example.c

# 默认目标
all: $(STATIC_LIB) $(SHARED_LIB) $(EXAMPLE)

# 静态库
$(STATIC_LIB): $(OBJECTS)
	ar rcs $@ $^

# 动态库
$(SHARED_LIB): $(OBJECTS)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# 示例程序
$(EXAMPLE): $(EXAMPLE_SRC) $(STATIC_LIB)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< -L. -llinx_alert -L../linx_thread -llinx_thread_pool -L../linx_rule_engine/rule_engine_match -loutput_match $(LDFLAGS)

# 编译目标文件
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 清理
clean:
	rm -f $(OBJECTS) $(STATIC_LIB) $(SHARED_LIB) $(EXAMPLE)

# 安装（可选）
install: $(STATIC_LIB) $(SHARED_LIB)
	mkdir -p /usr/local/lib
	mkdir -p /usr/local/include/linx
	cp $(STATIC_LIB) $(SHARED_LIB) /usr/local/lib/
	cp include/linx_alert.h /usr/local/include/linx/

# 测试
test: $(EXAMPLE)
	./$(EXAMPLE)

# 调试版本
debug: CFLAGS += -g -DDEBUG
debug: all

.PHONY: all clean install test debug
