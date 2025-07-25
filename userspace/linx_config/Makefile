# 子模块通用Makefile
MODULE_NAME ?= $(notdir $(CURDIR))

# 使用绝对路径确保可靠性
MODULE_DIR := $(CURDIR)
SRC_DIR := $(MODULE_DIR)
INCLUDE_DIR := $(MODULE_DIR)/include

# 构建目录定义
BUILD_DIR ?= $(TOPDIR)/build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_DIR := $(BUILD_DIR)/lib
LIBRARY := $(LIB_DIR)/lib$(MODULE_NAME).a

# 获取所有源文件
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/$(MODULE_NAME)/%.o,$(SRCS))

# 添加包含路径
CFLAGS += -I$(INCLUDE_DIR) -I$(TOPDIR)/include \
		  -I$(USR_DIR)/linx_yaml/include \
		  -I$(DEPENDS_DIR)/cJSON/include

.PHONY: all clean

all: $(LIBRARY)

$(LIBRARY): $(OBJS)
	@mkdir -p $(dir $@)
	@ar rcs $@ $^
	@echo "[AR library]: $@"

$(OBJ_DIR)/$(MODULE_NAME)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC]: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIBRARY)
	@rm -rf $(OBJ_DIR)/$(MODULE_NAME)
