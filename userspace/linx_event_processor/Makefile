# linx_event_processor Makefile

MODULE_NAME ?= linx_event_processor

# 目录定义
CURRENT_DIR := $(shell pwd)
SRC_DIR := $(CURRENT_DIR)
INCLUDE_DIR := $(CURRENT_DIR)/include
OBJ_DIR := $(BUILD_DIR)/obj/$(MODULE_NAME)
LIB_DIR := $(BUILD_DIR)/lib

# 源文件和目标文件
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
TARGET := $(LIB_DIR)/lib$(MODULE_NAME).a

# 编译选项 - 使用与主Makefile相同的包含路径
LOCAL_CFLAGS := $(CFLAGS) -I$(INCLUDE_DIR) \
			-I$(TOPDIR)/include \
			-I$(USR_DIR)/linx_arg_parser/include \
			-I$(USR_DIR)/linx_yaml/include \
			-I$(USR_DIR)/linx_config/include \
			-I$(USR_DIR)/linx_log/include \
			-I$(USR_DIR)/linx_rule_engine/include \
			-I$(USR_DIR)/linx_rule_engine/rule_engine_load/include \
			-I$(USR_DIR)/linx_rule_engine/rule_engine_match/include \
			-I$(USR_DIR)/linx_rule_engine/rule_engine_ast/include \
			-I$(USR_DIR)/linx_rule_engine/rule_engine_set/include \
			-I$(USR_DIR)/linx_engine/include \
			-I$(USR_DIR)/linx_alert/include/ \
			-I$(USR_DIR)/linx_event_rich/include/ \
			-I$(USR_DIR)/linx_event_queue/include/ \
			-I$(USR_DIR)/linx_thread/include/ \
			-I$(USR_DIR)/linx_process_cache/include/ \
			-I$(USR_DIR)/linx_apd/include/ \
			-I$(USR_DIR)/linx_hash_map/include \
			-I$(USR_DIR)/linx_machine_status/include/ \
			-I$(USR_DIR)/linx_control/include \
			-I$(DEPENDS_DIR)/uthash/include

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "[AR]: $@"
	@mkdir -p $(dir $@)
	@ar rcs $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "[CC]: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(LOCAL_CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)
	@rm -f $(TARGET)