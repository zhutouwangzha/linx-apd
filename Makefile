CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
INCLUDES = -I.
LIBS = 

# 源文件
SOURCES = field_mapper.c example.c rule_engine_example.c test_field_mapper.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = field_mapper_example
RULE_TARGET = rule_engine_example
TEST_TARGET = test_field_mapper

# 默认目标
all: $(TARGET) $(RULE_TARGET) $(TEST_TARGET)

# 编译可执行文件
$(TARGET): field_mapper.o example.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(RULE_TARGET): field_mapper.o rule_engine_example.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(TEST_TARGET): field_mapper.o test_field_mapper.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# 编译对象文件
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 清理
clean:
	rm -f $(OBJECTS) $(TARGET) $(RULE_TARGET) $(TEST_TARGET)

# 运行示例
run: $(TARGET)
	./$(TARGET)

# 运行规则引擎示例
run-rules: $(RULE_TARGET)
	./$(RULE_TARGET)

# 运行测试
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# 安装uthash（如果需要）
install-uthash:
	@echo "Checking for uthash..."
	@if [ ! -f "uthash.h" ]; then \
		echo "Downloading uthash..."; \
		wget -q https://raw.githubusercontent.com/troydhanson/uthash/master/src/uthash.h || \
		curl -s -o uthash.h https://raw.githubusercontent.com/troydhanson/uthash/master/src/uthash.h; \
		echo "uthash.h downloaded"; \
	else \
		echo "uthash.h already exists"; \
	fi

# 依赖关系
field_mapper.o: field_mapper.c field_mapper.h uthash.h
example.o: example.c field_mapper.h
rule_engine_example.o: rule_engine_example.c field_mapper.h
test_field_mapper.o: test_field_mapper.c field_mapper.h

.PHONY: all clean run run-rules test install-uthash