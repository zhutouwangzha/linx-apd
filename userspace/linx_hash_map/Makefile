# 子模块通用Makefile
MODULE_NAME ?= $(notdir $(CURDIR))

# 使用绝对路径确保可靠性
MODULE_DIR := $(CURDIR)
SRC_DIR := $(MODULE_DIR)
INCLUDE_DIR := $(MODULE_DIR)/include

# 构建目录定义
BUILD_DIR ?= ./build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_DIR := $(BUILD_DIR)/lib
LIBRARY := $(LIB_DIR)/lib$(MODULE_NAME).a

# 获取所有源文件（排除示例文件）
SRCS := $(filter-out $(SRC_DIR)/example_%.c, $(wildcard $(SRC_DIR)/*.c))
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/$(MODULE_NAME)/%.o,$(SRCS))

# 示例程序
EXAMPLE_MULTI_STRUCT := $(BUILD_DIR)/bin/example_multi_struct

# 添加包含路径
CFLAGS += -I$(INCLUDE_DIR) -I../../local_depends/uthash/include

.PHONY: all clean examples

all: $(LIBRARY)

examples: $(EXAMPLE_MULTI_STRUCT)

$(LIBRARY): $(OBJS)
	@mkdir -p $(dir $@)
	@ar rcs $@ $^
	@echo "[AR library]: $@"

$(OBJ_DIR)/$(MODULE_NAME)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC]: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(EXAMPLE_MULTI_STRUCT): $(SRC_DIR)/example_multi_struct.c $(LIBRARY)
	@mkdir -p $(dir $@)
	@echo "[CC example]: $<"
	@$(CC) $(CFLAGS) $< -L$(LIB_DIR) -l$(MODULE_NAME) -o $@

clean:
	@rm -f $(LIBRARY)
	@rm -f $(EXAMPLE_MULTI_STRUCT)
	@rm -rf $(OBJ_DIR)/$(MODULE_NAME)
