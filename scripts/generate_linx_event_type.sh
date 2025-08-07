#!/bin/bash

# 检查输入参数
if [[ $# -ne 2 ]]; then
	echo "Usage: $0 input_file output_file"
	echo "	Example: $0 linx_syscalls_macro.h interesting_syscalls.json"
	exit 1
else
	input_file="$1"
	output_file="$2"
fi

# 检查文件是否存在
if [ ! -f "$input_file" ]; then
	echo "Error: File '$input_file' not found."
	exit 1
fi

# 清空输出文件
> "$output_file"

function write_line()
{
	enter_number=$(echo "scale=1; $syscall_number * 2" | bc)
	exit_number=$(echo "scale=1; $syscall_number * 2 + 1" | bc)

    comm_line="    LINX_EVENT_TYPE_$syscall"
    event_enter_line="${comm_line}_E = $enter_number,"
    echo "$event_enter_line" >> $output_file

    event_exit_line="${comm_line}_X = $exit_number,"
    echo "$event_exit_line" >> $output_file
}

function generated_linx_event_type()
{
	# 创建初始结构
	echo "#ifndef __LINX_EVENT_TYPE_H__" >> "$output_file"
    echo "#define __LINX_EVENT_TYPE_H__" >> "$output_file"
    echo "" >> "$output_file"
    echo "typedef enum { " >> "$output_file"

	# 逐行匹配系统调用名和系统调用号
	while IFS= read -r line; do
		syscall=$(echo "$line" | sed -n 's/.*SYSCALL_MACRO([^,]*, \([^,]*\), \([^,]*\).*/\1/p' | tr '[:lower:]' '[:upper:]')
		syscall_number=$(echo "$line" | sed -n 's/.*SYSCALL_MACRO([^,]*, \([^,]*\), \([^,]*\).*/\2/p')

		# 判断变量是否为空
		if [[ -z "$syscall" && -z "$syscall_number" ]]; then
			continue;
		fi

		write_line
	done < "$input_file"

	echo "    LINX_EVENT_TYPE_MAX" >> "$output_file"
    echo "} linx_event_type_t;" >> "$output_file"
    echo "" >> "$output_file"
    echo "#endif /* __LINX_EVENT_TYPE_H__ */" >> "$output_file"

	echo "Linx event type have been written to '$output_file'."
}

generated_linx_event_type
