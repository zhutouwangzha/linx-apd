CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2
INCLUDES = -I./include -I../rule_engine_load/include

SRCDIR = .
INCDIR = include
OBJDIR = obj

# 源文件
SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# 目标库
TARGET = liblinx_rule_output.a

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

install: $(TARGET)
	@echo "Installing $(TARGET)"

.PHONY: install