#!/bin/bash

# 内存错误修复测试脚本

echo "=== 内存错误修复测试 ==="

# 编译项目
echo "1. 编译项目..."
make clean
make

if [ $? -ne 0 ]; then
    echo "编译失败"
    exit 1
fi

echo "2. 使用 valgrind 检测内存错误..."
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./userspace/linx_apd/linx_apd -c yaml_config/test.yaml -r yaml_config/test.yaml

echo "3. 使用 AddressSanitizer 编译和测试..."
export CFLAGS="-fsanitize=address -g"
export LDFLAGS="-fsanitize=address"
make clean
make

if [ $? -eq 0 ]; then
    echo "运行 AddressSanitizer 测试..."
    ./userspace/linx_apd/linx_apd -c yaml_config/test.yaml -r yaml_config/test.yaml
else
    echo "AddressSanitizer 编译失败"
fi

echo "4. 使用 ThreadSanitizer 编译和测试..."
export CFLAGS="-fsanitize=thread -g"
export LDFLAGS="-fsanitize=thread"
make clean
make

if [ $? -eq 0 ]; then
    echo "运行 ThreadSanitizer 测试..."
    ./userspace/linx_apd/linx_apd -c yaml_config/test.yaml -r yaml_config/test.yaml
else
    echo "ThreadSanitizer 编译失败"
fi

echo "=== 测试完成 ==="