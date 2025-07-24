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
BIN_DIR := $(BUILD_DIR)/bin
LIBRARY := $(LIB_DIR)/lib$(MODULE_NAME).a

# 测试程序
TEST_PROG := $(BIN_DIR)/test_$(MODULE_NAME)

# 获取所有源文件（排除测试文件）
SRCS := $(filter-out $(SRC_DIR)/test_*.c,$(wildcard $(SRC_DIR)/*.c))
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/$(MODULE_NAME)/%.o,$(SRCS))

# 测试源文件
TEST_SRCS := $(wildcard $(SRC_DIR)/test_*.c)
TEST_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/$(MODULE_NAME)/%.o,$(TEST_SRCS))

# 添加包含路径
CFLAGS +=	-I$(INCLUDE_DIR) -I$(TOPDIR)/include \
			-I$(USR_DIR)/linx_thread_pool/include \
			-I$(USR_DIR)/linx_hash_map/include \
			-I$(DEPENDS_DIR)/uthash/include

# 链接库
LDFLAGS += -L$(LIB_DIR) -l$(MODULE_NAME) -llinx_thread_pool -llinx_hash_map -lpthread

.PHONY: all clean test

all: $(LIBRARY)

test: $(TEST_PROG)

$(LIBRARY): $(OBJS)
	@mkdir -p $(dir $@)
	@ar rcs $@ $^
	@echo "[AR library]: $@"

$(TEST_PROG): $(TEST_OBJS) $(LIBRARY)
	@mkdir -p $(dir $@)
	@echo "[LD test]: $@"
	@$(CC) $(TEST_OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/$(MODULE_NAME)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "[CC]: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIBRARY) $(TEST_PROG)
	@rm -rf $(OBJ_DIR)/$(MODULE_NAME)
