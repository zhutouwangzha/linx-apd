# LINX Event Processor Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
LDFLAGS = -pthread

# 目录定义
INCLUDE_DIR = include
SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = bin

# 包含路径
INCLUDES = -I$(INCLUDE_DIR) \
           -I../linx_thread/include \
           -I../linx_log/include \
           -I../linx_rule_engine/rule_engine_set/include \
           -I../linx_rule_engine/rule_engine_match/include \
           -I../linx_alert/include \
           -I../linx_event_rich/include \
           -I../linx_engine/ebpf/include \
           -I../../include

# 源文件
SOURCES = linx_event_processor.c
EXAMPLE_SOURCES = example_usage.c

# 目标文件
OBJECTS = $(SOURCES:%.c=$(OBJ_DIR)/%.o)
EXAMPLE_OBJECTS = $(EXAMPLE_SOURCES:%.c=$(OBJ_DIR)/%.o)

# 库文件
LIBS = -L../linx_thread -llinx_thread \
       -L../linx_log -llinx_log \
       -L../linx_rule_engine/rule_engine_set -llinx_rule_engine_set \
       -L../linx_rule_engine/rule_engine_match -llinx_rule_engine_match \
       -L../linx_alert -llinx_alert \
       -L../linx_event_rich -llinx_event_rich \
       -L../linx_engine/ebpf -llinx_ebpf

# 目标
TARGET_LIB = $(BIN_DIR)/liblinx_event_processor.a
TARGET_SHARED = $(BIN_DIR)/liblinx_event_processor.so
TARGET_EXAMPLE = $(BIN_DIR)/example_usage

.PHONY: all clean static shared example install

# 默认目标
all: static shared example

# 静态库
static: $(TARGET_LIB)

# 动态库
shared: $(TARGET_SHARED)

# 示例程序
example: $(TARGET_EXAMPLE)

# 创建目录
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# 编译目标文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -fPIC -c $< -o $@

# 生成静态库
$(TARGET_LIB): $(OBJECTS) | $(BIN_DIR)
	ar rcs $@ $(OBJECTS)
	@echo "Static library created: $@"

# 生成动态库
$(TARGET_SHARED): $(OBJECTS) | $(BIN_DIR)
	$(CC) -shared -o $@ $(OBJECTS) $(LDFLAGS)
	@echo "Shared library created: $@"

# 编译示例程序
$(TARGET_EXAMPLE): $(EXAMPLE_OBJECTS) $(TARGET_LIB) | $(BIN_DIR)
	$(CC) -o $@ $(EXAMPLE_OBJECTS) -L$(BIN_DIR) -llinx_event_processor $(LIBS) $(LDFLAGS)
	@echo "Example program created: $@"

# 安装
install: static shared
	@echo "Installing LINX Event Processor..."
	@mkdir -p /usr/local/include/linx
	@mkdir -p /usr/local/lib
	@cp $(INCLUDE_DIR)/*.h /usr/local/include/linx/
	@cp $(TARGET_LIB) /usr/local/lib/
	@cp $(TARGET_SHARED) /usr/local/lib/
	@ldconfig
	@echo "Installation completed."

# 清理
clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned up build artifacts."

# 重新编译
rebuild: clean all

# 调试信息
debug: CFLAGS += -DDEBUG -O0
debug: all

# 性能优化版本
release: CFLAGS += -DNDEBUG -O3 -flto
release: LDFLAGS += -flto
release: all

# 代码格式化
format:
	@find . -name "*.c" -o -name "*.h" | xargs clang-format -i
	@echo "Code formatted."

# 静态分析
analyze:
	@cppcheck --enable=all --inconclusive --std=c99 $(INCLUDES) $(SOURCES)

# 帮助信息
help:
	@echo "Available targets:"
	@echo "  all        - Build static library, shared library, and example"
	@echo "  static     - Build static library only"
	@echo "  shared     - Build shared library only"
	@echo "  example    - Build example program only"
	@echo "  install    - Install libraries and headers"
	@echo "  clean      - Remove build artifacts"
	@echo "  rebuild    - Clean and rebuild"
	@echo "  debug      - Build with debug information"
	@echo "  release    - Build optimized release version"
	@echo "  format     - Format source code"
	@echo "  analyze    - Run static analysis"
	@echo "  help       - Show this help message"

# 依赖关系
$(OBJECTS): $(INCLUDE_DIR)/linx_event_processor.h $(INCLUDE_DIR)/linx_event_processor_config.h
$(EXAMPLE_OBJECTS): $(INCLUDE_DIR)/linx_event_processor.h