CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -O2
INCLUDES = -I./include -I../rule_engine_load/include

SRCDIR = .
INCDIR = include
OBJDIR = obj

# 源文件（排除测试文件）
SRCS = $(filter-out test_%.c, $(wildcard $(SRCDIR)/*.c))
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# 目标库
TARGET = liblinx_rule_output.a

# 测试程序
TEST_SRCS = $(wildcard test_*.c)
TEST_TARGETS = $(TEST_SRCS:%.c=%)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	ar rcs $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

# 编译测试程序
test_%: test_%.c $(TARGET)
	$(CC) $(CFLAGS) $(INCLUDES) $< -L. -llinx_rule_output -o $@

test: test_efficient_output
	./test_efficient_output

clean:
	rm -rf $(OBJDIR) $(TARGET) $(TEST_TARGETS)

install: $(TARGET)
	@echo "Installing $(TARGET)"

.PHONY: install