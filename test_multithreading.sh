#!/bin/bash

# 测试多线程事件处理器的脚本
echo "=== Linx-APD 多线程测试 ==="

# 检查是否在正确的目录
if [ ! -f "Makefile" ]; then
    echo "错误: 请在项目根目录运行此脚本"
    exit 1
fi

# 编译项目
echo "1. 编译项目..."
make clean
make linx_apd -j$(nproc)

if [ $? -ne 0 ]; then
    echo "编译失败！"
    exit 1
fi

echo "编译成功！"

# 检查可执行文件
if [ ! -f "build/bin/linx-apd" ]; then
    echo "错误: 可执行文件未找到"
    exit 1
fi

echo "2. 检查多线程配置..."
echo "当前配置:"
echo "  - 事件处理器: 启用"
echo "  - 获取线程数: CPU核心数 (自动)"
echo "  - 匹配线程数: CPU核心数*2 (自动)"

echo ""
echo "3. 启动测试 (按 Ctrl+C 停止)..."
echo "注意: 需要 root 权限运行 eBPF 程序"

# 使用测试配置运行
sudo ./build/bin/linx-apd -c yaml_config/linx_apd_config/linx_apd.yaml -r yaml_config/linx_apd_rules/read_passwd.yaml

echo ""
echo "测试完成！"