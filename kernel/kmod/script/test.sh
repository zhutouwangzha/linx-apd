#!/bin/bash

# 脚本路径设置
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
ROOT_DIR=$(dirname "$SCRIPT_DIR")
USER_APP="$ROOT_DIR/user/app"


# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # 无颜色

# 检查内核模块是否加载
check_module() {
    if ! lsmod | grep -q "sysmon"; then
        echo -e "${RED}错误: sysmon.ko 内核模块未加载!${NC}"
        exit 1
    fi
    echo -e "${GREEN}模块已加载.${NC}"
}


# 功能测试
file_test() {
    echo "=== 开始文件测试 ===="
    local test_file="1.txt"

    touch "$test_file"
    if [ -f "$test_file" ]; then
        echo -e "${GREEN}文件创建成功!${NC}"
    else
        echo -e "${RED}文件创建失败!${NC}"
        return 1
    fi
    
    rm -f "$test_file"
    if [ ! -f "$test_file" ]; then
        echo -e "${GREEN}文件删除成功!${NC}"
    else
        echo -e "${RED}文件删除失败!${NC}"
        return 1
    fi
    echo "=== 文件测试完成 ===="
    return 0

}

app_test() {
    echo "=== 开始应用程序测试 ===="

    if [ ! -x "$USER_APP" ]; then
        echo -e "${RED}应用程序不存在或不可执行!${NC}"
        return 1
    fi
    "$USER_APP"
}

main() {
    check_module

    if ! file_test; then
        echo -e "${RED}文件测试失败!${NC}"
        exit 2 
    fi

    app_test
}

main


