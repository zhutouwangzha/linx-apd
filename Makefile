TOPDIR	= $(shell pwd)

BUILD_DIR = $(TOPDIR)/build
LIB_DIR := $(BUILD_DIR)/lib
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj
USR_DIR := $(TOPDIR)/userspace
DEPENDS_DIR := $(TOPDIR)/local_depends

LINX_APD_DIR := $(USR_DIR)/linx_apd

CC 		:= gcc
INCLUDE := -I$(TOPDIR)/include -I$(USR_DIR)/linx_arg_parser/include \
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
			-I$(USR_DIR)/linx_hash_map/include

CFLAGS 	:= -Wall -Wextra -g $(INCLUDE) \
		   -DPCRE2_CODE_UNIT_WIDTH=8		# 这是pcre2库的编译选项，指定UTF8编码
LDFLAGS := -lpthread -lyaml -lpcre2-8

# 三方库目录
LOCAL_LIB_DIR := -L$(DEPENDS_DIR)/libyaml/libs \
				 -L$(DEPENDS_DIR)/pcre2/libs

# 获取所有包含Makefile的userspace子目录(排除linx_apd)
USERSPACE_DIRS := $(shell find $(USR_DIR) -mindepth 1 -maxdepth 1 -type d)
LIBRARY_DIRS := $(filter-out $(LINX_APD_DIR), \
				$(foreach dir,$(USERSPACE_DIRS), \
				$(if $(wildcard $(dir)/Makefile),$(dir),)))

# 生成库文件列表（liblinx_xxx.a）
LIBRARIES := $(addprefix $(LIB_DIR)/lib,$(addprefix .a,$(notdir $(LIBRARY_DIRS)))))
LINK_LIBS := $(addprefix -l,$(notdir $(LIBRARY_DIRS)))

# 主程序文件
LINX_APD_SRCS := $(wildcard $(LINX_APD_DIR)/*.c)
LINX_APD_OBJS := $(patsubst $(LINX_APD_DIR)/%.c, $(OBJ_DIR)/linx_apd/%.o, $(LINX_APD_SRCS))
EXECUTABLE := $(BIN_DIR)/linx-apd

export TOPDIR CC CFLAGS LDFLAGS BUILD_DIR DEPENDS_DIR USR_DIR

.PHONY: all clean $(LIBRARY_DIRS)

all: $(EXECUTABLE)

# 链接可执行文件
$(EXECUTABLE): $(LIBRARIES) $(LINX_APD_OBJS)
	@echo "[Link]: $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(LINX_APD_OBJS) -L$(LIB_DIR) $(LOCAL_LIB_DIR) $(LINK_LIBS) $(LDFLAGS) -o $@

# 编译主程序
$(OBJ_DIR)/linx_apd/%.o: $(LINX_APD_DIR)/%.c
	@echo "[CC]: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# 递归构建子目录库
$(LIBRARIES): $(LIBRARY_DIRS)
	@true  # 实际工作由子目录规则完成

# 子目录构建规则
$(LIBRARY_DIRS):
	@echo "[Build module]: $(notdir $@)"
	@$(MAKE) --no-print-directory -C $@ MODULE_NAME=$(notdir $@)

clean:
	@echo "[Cleaning build...]"
	@for dir in $(LIBRARY_DIRS); do \
		echo "[Cleaning module]: $$(basename $$dir)"; \
		$(MAKE) --no-print-directory -C $$dir clean; \
	done
	@rm -rf $(OBJ_DIR)/linx_apd
	@rm -rf $(BUILD_DIR)
	@echo "[Clea complete]"
