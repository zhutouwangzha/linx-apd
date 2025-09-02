MODULE_NAME ?= linx_queue

USR_DIR := $(TOPDIR)/userspace
BUILD_DIR := $(TOPDIR)/build
LIB_DIR := $(BUILD_DIR)/lib
OBJ_DIR := $(BUILD_DIR)/obj

CC := gcc
INCLUDE := -I$(TOPDIR)/include -Iinclude
CFLAGS := -Wall -Wextra -g -fPIC $(INCLUDE)
LDFLAGS := -lpthread

# 源文件
SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c, $(OBJ_DIR)/$(MODULE_NAME)/%.o, $(SOURCES))
LIBRARY := $(LIB_DIR)/lib$(MODULE_NAME).a

.PHONY: all clean

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	@echo "[AR]: $@"
	@mkdir -p $(dir $@)
	@ar rcs $@ $^

$(OBJ_DIR)/$(MODULE_NAME)/%.o: %.c
	@echo "[CC]: $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf $(OBJ_DIR)/$(MODULE_NAME)
	@rm -f $(LIBRARY)