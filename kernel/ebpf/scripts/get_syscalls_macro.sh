#!/bin/bash

syscalls_file="/usr/include/asm/unistd_64.h"

# 检查输入参数
if [[ $# -ne 1 ]]; then
	echo "Usage: $0 output_file"
	exit 1
else
	output_file="$1"
fi

# 检查文件是否存在
if [ ! -f "$syscalls_file" ]; then
	echo "Error: File '$syscalls_file' not found."
	exit 1
fi

# 获取当前系统架构
arch_macro="__x86_64__"

# 清空输出文件
> "$output_file"

declare -a enter_param_types
declare -a enter_param_names
declare -a exit_param_types
declare -a exit_param_names

function write_line()
{
    output_line="SYSCALL_MACRO($syscall_upper, $syscall, $syscall_number, $enter_param_count, $exit_param_count)"

    output_line+=" ENTER_PARAM_MACRO("
    for ((i=0; i<enter_param_count; ++i)); do
        output_line+="${enter_param_types[i]}, ${enter_param_names[i]}"
        if ((i!=enter_param_count-1)); then
            output_line+=", "
        fi
    done
    output_line+=")"


    output_line+=" EXIT_PARAM_MACRO("
    for ((i=0; i<exit_param_count; ++i)); do
        output_line+="${exit_param_types[i]}, ${exit_param_names[i]}"
        if ((i!=exit_param_count-1)); then
            output_line+=", "
        fi
    done
    output_line+=")"

    echo $output_line >> $output_file
}

function get_syscall_enter_param()
{
    enter_null_line_number=0
    enter_param_count=0
    unset enter_param_types
    unset enter_param_names
    exit_param_count=0
    unset exit_param_types
    unset exit_param_names

    syscall_enter_file="/sys/kernel/debug/tracing/events/syscalls/sys_enter_$syscall/format"
    if [[ ! -f "$syscall_enter_file" ]]; then
        # 从 man 手册获取系统调用的函数签名
        man_mesage=$(man -ch 2 $syscall 2>/dev/null | col -bx | grep -v '^$')
        exit_status=${PIPESTATUS[0]}

        if [[ $exit_status -eq 0 && $man_mesage != *"Unimplemented system calls."* ]]; then
            # 预处理数据，合并多行函数签名
            preprocessed_data=$(echo "$man_mesage" | awk -v target="$syscall" -v arch_macro="$arch_macro" '
            {
                # 移除可能的属性标签
                gsub(/\[\[.*\]\]/, "")

                # 合并多行：如果以逗号结尾，则继续下一行
                if (NR > 1 && last_line !~ /[;)]$/) {
                    last_line = last_line " " $0
                } else {
                    if (NR > 1) {
                        # 只保留包含系统调用函数名的行
                        if (last_line ~ target) {
                            if (last_line !~ "\\<necessitating\\>") {
                                # 只保留特定框架的行
                                if (last_line ~ arch_macro) {
                                    print last_line
                                } else if (last_line !~ /__\w+__/) {
                                    print last_line
                                }
                            }
                        }
                    }
                    last_line = $0
                }
            }
            END {
                    # 处理最后一行
                    if (last_line ~ target) {
                        if (last_line !~ "\\<necessitating\\>") {
                            # 只保留特定框架的行
                            if (last_line ~ arch_macro) {
                                print last_line
                            } else if (last_line !~ /__\w+__/) {
                                print last_line
                            }
                        }
                    }
            }')

            # 处理每个函数签名
            while IFS= read -r line; do
                # 提取参数列表
                if [[ $line =~ [^\(]*\((.*)\) ]]; then
                    param_str=${BASH_REMATCH[1]}

                    # 特殊处理 syscall 宏，跳过第一个参数
                    if [[ $line =~ syscall\( ]]; then
                        # 移除第一个逗号前的所有内容
                        param_str=${param_str#*,}
                    fi

                    # 分割处理每个参数
                    IFS=',' read -ra params <<< "$param_str"
                    for param in "${params[@]}"; do
                        # 使用正则表达式提取类型和参数名
                        if [[ $param =~ [[:space:]]*(const|volatile|struct|union|enum|const struct)?[[:space:]]*([^*&[:space:]]+[[:space:]]*[*&]?)[[:space:]]+([^[:space:],]+) ]]; then
                            modifier=${BASH_REMATCH[1]}
                            base_type=${BASH_REMATCH[2]}
                            param_name=${BASH_REMATCH[3]}

                            # 构建完整类型
                            full_type=""
                            [[ -n $modifier ]] && full_type+="$modifier "
                            
                            # 移除类型中的空格
                            full_type+="${base_type// /}"

                            # 将参数名称中的指针符号移动到参数类型中
                            if [[ $param_name == *"*"* ]]; then
                                full_type+=" *"
                                param_name="${param_name#\*}"
                            fi

                            # 特殊处理，将第一个 [ 后的所有字符串删除
                            if [[ $syscall == "query_module" && "$full_type" == "void" ]]; then
                                full_type+=" *"
                                param_name="${param_name%%[*}"
                            fi

                            enter_param_types+=("$full_type")
                            enter_param_names+=("$param_name")
                            ((enter_param_count++))
                        else
                            echo "通过 man 无法解析 '$syscall' 系统调用的参数"
                            return
                        fi
                    done
                fi

                # 特殊处理，读到第一个后直接跳过
                if [[ $syscall == "set_thread_area" ]]; then
                    break;
                fi
            done <<< "$preprocessed_data"

            # 通过 man 手册获取的系统调用参数，默认将退出参数类型和参数名称固定为 long ret
            exit_param_types+=("long")
            exit_param_names+=("ret")
            ((exit_param_count++))
        fi

        return
    fi

    while IFS= read -r line; do
        # 记录空行
        if [ -z "$line" ]; then
            ((enter_null_line_number++))
            continue;
        fi

        if [ $enter_null_line_number = 0 ]; then
            continue;
        fi

        if [[ "$line" == *"__syscall_nr"* ]]; then
            continue;
        fi

        if [[ "$line" =~ field:([a-zA-Z0-9_* ]+)[[:space:]]([a-zA-Z0-9_]+)\; ]]; then
            enter_param_type="${BASH_REMATCH[1]}"
            enter_param_name="${BASH_REMATCH[2]}"

            enter_param_types+=("$enter_param_type")
            enter_param_names+=("$enter_param_name")
            ((enter_param_count++))
        fi
    done < "$syscall_enter_file"
}

function get_syscall_exit_param()
{
    # 优先判断文件是否存在
    syscall_exit_file="/sys/kernel/debug/tracing/events/syscalls/sys_exit_$syscall/format"
    if [[ ! -f "$syscall_exit_file" ]]; then
        return
    fi

    # 复位全局变量的值
    exit_null_line_number=0
    exit_param_count=0
    unset exit_param_types
    unset exit_param_names

    while IFS= read -r line; do
        # 记录空行
        if [ -z "$line" ]; then
            ((exit_null_line_number++))
            continue;
        fi

        if [ $exit_null_line_number = 0 ]; then
            continue;
        fi

        if [[ "$line" == *"__syscall_nr"* ]]; then
            continue;
        fi

        if [[ "$line" =~ field:([a-zA-Z0-9_* ]+)[[:space:]]([a-zA-Z0-9_]+)\; ]]; then
            exit_param_type="${BASH_REMATCH[1]}"
            exit_param_name="${BASH_REMATCH[2]}"

            exit_param_types+=("$exit_param_type")
            exit_param_names+=("$exit_param_name")
            ((exit_param_count++))
        fi
    done < "$syscall_exit_file"
}

function get_syscall_param()
{
    if [[ "$syscall" = "stat" || "$syscall" = "fstat" ]]; then
        syscall="${syscall}fs"
        get_syscall_enter_param
        get_syscall_exit_param
        syscall="${syscall%fs}"
    elif [[ "$syscall" = "lstat" ]]; then
        syscall="${syscall#l}fs"
        get_syscall_enter_param
        get_syscall_exit_param
        syscall="l${syscall%fs}"
    elif [[ "$syscall" = "sendfile" ]]; then
        syscall="${syscall}64"
        get_syscall_enter_param
        get_syscall_exit_param
        syscall="${syscall%64}"
    elif [[ "$syscall" = "uname" ]]; then
        syscall="new${syscall}"
        get_syscall_enter_param
        get_syscall_exit_param
        syscall="${syscall#new}"
    else 
        get_syscall_enter_param
        get_syscall_exit_param
    fi
}

function get_syscalls()
{
    echo "/* This file was generated by scripts/get_syscalls_macro.sh. Please do not modify it! */" >> "$output_file"

    # 获取所有系统调用的名称
    syscalls=$(grep -E '#define __NR_' $syscalls_file | awk '{print$2}' | sed 's/__NR_//')

    for syscall in $syscalls; do
        # 大写系统调用名称
        syscall_upper="${syscall^^}"

        # 获取系统调用号
        syscall_number=$(grep -E "#define __NR_$syscall\b" $syscalls_file | awk '{print$3}')

        get_syscall_param

        # 入参和出参全为0，表示找不到该系统调用的函数签名，跳过该系统调用
        if [[ "$enter_param_count" -eq 0 && "$exit_param_count" -eq 0 ]]; then
            echo "Discard '$syscall' system calls"
            continue;
        fi

        write_line
    done

    echo "#undef SYSCALL_MACRO" >> "$output_file"
    echo "#undef ENTER_PARAM_MACRO" >> "$output_file"
    echo "#undef EXIT_PARAM_MACRO" >> "$output_file"
    echo "Extracted system calls have been written to '$output_file'."
}

get_syscalls