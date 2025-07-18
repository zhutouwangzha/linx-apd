#!/bin/bash

# 多线程事件处理测试脚本

echo "=== 多线程事件处理测试 ==="

# 检查CPU核心数
CPU_CORES=$(nproc)
echo "检测到 $CPU_CORES 个CPU核心"

# 编译项目
echo "1. 编译项目..."
make clean
make

if [ $? -ne 0 ]; then
    echo "编译失败"
    exit 1
fi

echo "2. 启动多线程事件处理器..."
echo "   - 事件获取线程数: $CPU_CORES"
echo "   - 规则匹配线程数: $((CPU_CORES * 2))"

# 运行程序并捕获输出
timeout 30s ./userspace/linx_apd/linx_apd -c yaml_config/test.yaml -r yaml_config/test.yaml &
PID=$!

echo "3. 程序已启动 (PID: $PID)，将运行30秒..."

# 等待程序结束或超时
wait $PID
EXIT_CODE=$?

if [ $EXIT_CODE -eq 124 ]; then
    echo "程序正常运行30秒后被终止"
elif [ $EXIT_CODE -eq 0 ]; then
    echo "程序正常退出"
else
    echo "程序异常退出，退出码: $EXIT_CODE"
fi

echo "4. 检查系统资源使用情况..."
echo "CPU使用率:"
top -b -n1 | grep "Cpu(s)" || echo "无法获取CPU使用率"

echo "内存使用情况:"
free -h || echo "无法获取内存使用情况"

echo "5. 检查进程和线程..."
echo "当前系统中的linx_apd进程:"
ps aux | grep linx_apd | grep -v grep || echo "没有找到linx_apd进程"

echo "=== 测试完成 ==="

# 清理可能残留的进程
pkill -f linx_apd 2>/dev/null || true