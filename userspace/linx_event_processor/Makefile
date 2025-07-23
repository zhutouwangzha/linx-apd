# LINX Event Processor Makefile - Advanced Version

CC = gcc
CFLAGS = -Wall -Wextra -std=gnu99 -O2 -g -D_GNU_SOURCE
LDFLAGS = -pthread -lrt

# 目录定义
INCLUDE_DIR = include
SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = bin
TEST_DIR = tests

# 包含路径
INCLUDES = -I$(INCLUDE_DIR) \
           -I../linx_log/include \
           -I../linx_rule_engine/rule_engine_set/include \
           -I../linx_rule_engine/rule_engine_match/include \
           -I../linx_rule_engine/rule_engine_load/include \
           -I../linx_alert/include \
           -I../linx_event_rich/include \
           -I../linx_engine/ebpf/include \
           -I../../include

# 源文件
SOURCES = linx_event_processor.c
SIMPLE_EXAMPLE_SOURCES = example_usage.c
ADVANCED_EXAMPLE_SOURCES = advanced_example.c

# 目标文件
OBJECTS = $(SOURCES:%.c=$(OBJ_DIR)/%.o)
SIMPLE_EXAMPLE_OBJECTS = $(SIMPLE_EXAMPLE_SOURCES:%.c=$(OBJ_DIR)/%.o)
ADVANCED_EXAMPLE_OBJECTS = $(ADVANCED_EXAMPLE_SOURCES:%.c=$(OBJ_DIR)/%.o)

# 库文件（如果需要链接外部库）
LIBS = -L../linx_log -llinx_log \
       -L../linx_rule_engine/rule_engine_set -llinx_rule_engine_set \
       -L../linx_rule_engine/rule_engine_match -llinx_rule_engine_match \
       -L../linx_alert -llinx_alert \
       -L../linx_event_rich -llinx_event_rich \
       -L../linx_engine/ebpf -llinx_ebpf

# 目标
TARGET_LIB = $(BIN_DIR)/liblinx_event_processor.a
TARGET_SHARED = $(BIN_DIR)/liblinx_event_processor.so
TARGET_SIMPLE_EXAMPLE = $(BIN_DIR)/simple_example
TARGET_ADVANCED_EXAMPLE = $(BIN_DIR)/advanced_example

# 测试相关
TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(OBJ_DIR)/test_%.o)
TEST_TARGETS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(BIN_DIR)/%)

.PHONY: all clean static shared examples simple advanced tests install uninstall format analyze benchmark help

# 默认目标
all: static shared examples

# 静态库
static: $(TARGET_LIB)

# 动态库
shared: $(TARGET_SHARED)

# 所有示例
examples: simple advanced

# 简单示例
simple: $(TARGET_SIMPLE_EXAMPLE)

# 高级示例
advanced: $(TARGET_ADVANCED_EXAMPLE)

# 测试
tests: $(TEST_TARGETS)

# 创建目录
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(TEST_DIR):
	@mkdir -p $(TEST_DIR)

# 编译目标文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) $(INCLUDES) -fPIC -c $< -o $@

# 编译测试文件
$(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c | $(OBJ_DIR)
	@echo "Compiling test $<..."
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 生成静态库
$(TARGET_LIB): $(OBJECTS) | $(BIN_DIR)
	@echo "Creating static library $@..."
	@ar rcs $@ $(OBJECTS)
	@echo "✓ Static library created: $@"

# 生成动态库
$(TARGET_SHARED): $(OBJECTS) | $(BIN_DIR)
	@echo "Creating shared library $@..."
	@$(CC) -shared -o $@ $(OBJECTS) $(LDFLAGS)
	@echo "✓ Shared library created: $@"

# 编译简单示例程序
$(TARGET_SIMPLE_EXAMPLE): $(SIMPLE_EXAMPLE_OBJECTS) $(TARGET_LIB) | $(BIN_DIR)
	@echo "Creating simple example $@..."
	@$(CC) -o $@ $(SIMPLE_EXAMPLE_OBJECTS) -L$(BIN_DIR) -llinx_event_processor $(LIBS) $(LDFLAGS)
	@echo "✓ Simple example created: $@"

# 编译高级示例程序
$(TARGET_ADVANCED_EXAMPLE): $(ADVANCED_EXAMPLE_OBJECTS) $(TARGET_LIB) | $(BIN_DIR)
	@echo "Creating advanced example $@..."
	@$(CC) -o $@ $(ADVANCED_EXAMPLE_OBJECTS) -L$(BIN_DIR) -llinx_event_processor $(LIBS) $(LDFLAGS)
	@echo "✓ Advanced example created: $@"

# 编译测试程序
$(BIN_DIR)/%: $(OBJ_DIR)/test_%.o $(TARGET_LIB) | $(BIN_DIR)
	@echo "Creating test $@..."
	@$(CC) -o $@ $< -L$(BIN_DIR) -llinx_event_processor $(LIBS) $(LDFLAGS)

# 安装
install: static shared
	@echo "Installing LINX Event Processor..."
	@sudo mkdir -p /usr/local/include/linx
	@sudo mkdir -p /usr/local/lib
	@sudo cp $(INCLUDE_DIR)/*.h /usr/local/include/linx/
	@sudo cp $(TARGET_LIB) /usr/local/lib/
	@sudo cp $(TARGET_SHARED) /usr/local/lib/
	@sudo ldconfig
	@echo "✓ Installation completed."

# 卸载
uninstall:
	@echo "Uninstalling LINX Event Processor..."
	@sudo rm -f /usr/local/include/linx/linx_event_processor*.h
	@sudo rm -f /usr/local/lib/liblinx_event_processor.*
	@sudo ldconfig
	@echo "✓ Uninstallation completed."

# 清理
clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "✓ Cleaned up build artifacts."

# 重新编译
rebuild: clean all

# 调试版本
debug: CFLAGS += -DDEBUG -O0 -ggdb3 -fsanitize=address -fsanitize=undefined
debug: LDFLAGS += -fsanitize=address -fsanitize=undefined
debug: all

# 性能优化版本
release: CFLAGS += -DNDEBUG -O3 -flto -march=native
release: LDFLAGS += -flto
release: all

# 代码覆盖率版本
coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += -lgcov
coverage: all

# 代码格式化
format:
	@echo "Formatting source code..."
	@find . -name "*.c" -o -name "*.h" | grep -v $(OBJ_DIR) | grep -v $(BIN_DIR) | xargs clang-format -i
	@echo "✓ Code formatted."

# 静态分析
analyze:
	@echo "Running static analysis..."
	@cppcheck --enable=all --inconclusive --std=c99 $(INCLUDES) --suppress=missingIncludeSystem $(SOURCES)
	@echo "✓ Static analysis completed."

# 内存检查
valgrind: $(TARGET_ADVANCED_EXAMPLE)
	@echo "Running memory check with Valgrind..."
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(TARGET_ADVANCED_EXAMPLE) --help

# 性能分析
perf: $(TARGET_ADVANCED_EXAMPLE)
	@echo "Running performance analysis..."
	@perf record -g $(TARGET_ADVANCED_EXAMPLE) --fetcher-threads 2 --matcher-threads 4
	@perf report

# 基准测试
benchmark: $(TARGET_ADVANCED_EXAMPLE)
	@echo "Running benchmark..."
	@echo "Small load (2 fetchers, 4 matchers):"
	@timeout 30s $(TARGET_ADVANCED_EXAMPLE) --fetcher-threads 2 --matcher-threads 4 || true
	@echo ""
	@echo "Medium load (4 fetchers, 8 matchers):"
	@timeout 30s $(TARGET_ADVANCED_EXAMPLE) --fetcher-threads 4 --matcher-threads 8 || true
	@echo ""
	@echo "High load (8 fetchers, 16 matchers):"
	@timeout 30s $(TARGET_ADVANCED_EXAMPLE) --fetcher-threads 8 --matcher-threads 16 || true

# 运行单元测试
test: tests
	@echo "Running unit tests..."
	@for test in $(TEST_TARGETS); do \
		echo "Running $$test..."; \
		$$test || exit 1; \
	done
	@echo "✓ All tests passed."

# 生成文档
docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile 2>/dev/null || echo "Doxygen not found, skipping documentation generation"

# 检查依赖
deps:
	@echo "Checking dependencies..."
	@ldd $(TARGET_SHARED) 2>/dev/null || echo "Shared library not built yet"

# 创建发布包
package: clean all
	@echo "Creating release package..."
	@mkdir -p linx_event_processor_release
	@cp -r $(INCLUDE_DIR) linx_event_processor_release/
	@cp $(TARGET_LIB) $(TARGET_SHARED) linx_event_processor_release/
	@cp $(TARGET_SIMPLE_EXAMPLE) $(TARGET_ADVANCED_EXAMPLE) linx_event_processor_release/
	@cp README.md linx_event_processor_release/
	@tar czf linx_event_processor_release.tar.gz linx_event_processor_release/
	@rm -rf linx_event_processor_release/
	@echo "✓ Release package created: linx_event_processor_release.tar.gz"

# 帮助信息
help:
	@echo "LINX Event Processor Build System"
	@echo "================================="
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build static library, shared library, and examples"
	@echo "  static        - Build static library only"
	@echo "  shared        - Build shared library only"
	@echo "  examples      - Build all example programs"
	@echo "  simple        - Build simple example program"
	@echo "  advanced      - Build advanced example program"
	@echo "  tests         - Build all test programs"
	@echo ""
	@echo "Installation:"
	@echo "  install       - Install libraries and headers to system"
	@echo "  uninstall     - Remove installed files from system"
	@echo ""
	@echo "Build variants:"
	@echo "  debug         - Build with debug symbols and sanitizers"
	@echo "  release       - Build optimized release version"
	@echo "  coverage      - Build with code coverage instrumentation"
	@echo ""
	@echo "Development:"
	@echo "  clean         - Remove build artifacts"
	@echo "  rebuild       - Clean and rebuild"
	@echo "  format        - Format source code"
	@echo "  analyze       - Run static analysis"
	@echo "  test          - Run unit tests"
	@echo ""
	@echo "Analysis and debugging:"
	@echo "  valgrind      - Run memory check with Valgrind"
	@echo "  perf          - Run performance analysis"
	@echo "  benchmark     - Run benchmark tests"
	@echo ""
	@echo "Utilities:"
	@echo "  docs          - Generate documentation"
	@echo "  deps          - Check library dependencies"
	@echo "  package       - Create release package"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make debug examples    - Build debug version with examples"
	@echo "  make release install   - Build and install release version"
	@echo "  make test benchmark    - Run tests and benchmarks"

# 依赖关系
$(OBJECTS): $(INCLUDE_DIR)/linx_event_processor.h
$(SIMPLE_EXAMPLE_OBJECTS): $(INCLUDE_DIR)/linx_event_processor.h
$(ADVANCED_EXAMPLE_OBJECTS): $(INCLUDE_DIR)/linx_event_processor.h

# 自动生成头文件依赖
-include $(OBJECTS:.o=.d)
-include $(SIMPLE_EXAMPLE_OBJECTS:.o=.d)
-include $(ADVANCED_EXAMPLE_OBJECTS:.o=.d)

$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@$(CC) $(CFLAGS) $(INCLUDES) -MM -MT $(@:.d=.o) $< > $@