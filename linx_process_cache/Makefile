# Makefile for linx_process_cache module

CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -D_GNU_SOURCE
LDFLAGS = -pthread

# Include paths
INCLUDES = -I./include \
           -I../local_depends/uthash/include \
           -I../userspace/linx_thread/include \
           -I../include

# Source files
SRCS = linx_process_cache.c
TEST_SRCS = test_process_cache.c

# Object files
OBJS = $(SRCS:.c=.o)
TEST_OBJS = $(TEST_SRCS:.c=.o)

# Thread pool dependency
THREAD_POOL_OBJ = ../userspace/linx_thread/linx_thread_pool.o

# Target library
LIB = liblinx_process_cache.a

# Test executable
TEST_BIN = test_process_cache

# Default target
all: $(LIB) $(TEST_BIN)

# Build static library
$(LIB): $(OBJS)
	ar rcs $@ $^

# Build test executable
$(TEST_BIN): $(TEST_OBJS) $(LIB) $(THREAD_POOL_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ 

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build thread pool dependency
$(THREAD_POOL_OBJ):
	make -C ../userspace/linx_thread linx_thread_pool.o

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TEST_OBJS) $(LIB) $(TEST_BIN)

# Run test
test: $(TEST_BIN)
	./$(TEST_BIN)

# Run monitor mode
monitor: $(TEST_BIN)
	./$(TEST_BIN) monitor

.PHONY: all clean test monitor