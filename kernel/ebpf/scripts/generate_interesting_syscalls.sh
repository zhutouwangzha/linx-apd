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
	# 生成时，默认不采集该系统调用，需要手动将 interesting 修改为 1
	jq --arg name "$syscall" \
	   --argjson id $syscall_number \
    	'. + {($name): {"syscall_id": $id, "interesting": 0}}' \
		$output_file > tmp && mv tmp $output_file
}

function generated_interesting_syscalls()
{
	# 创建初始结构
	echo '{}' > $output_file

	# 逐行匹配系统调用名和系统调用号
	while IFS= read -r line; do
		syscall=$(echo "$line" | sed -n 's/.*SYSCALL_MACRO([^,]*, \([^,]*\), \([^,]*\).*/\1/p')
		syscall_number=$(echo "$line" | sed -n 's/.*SYSCALL_MACRO([^,]*, \([^,]*\), \([^,]*\).*/\2/p')

		# 判断变量是否为空
		if [[ -z "$syscall" && -z "$syscall_number" ]]; then
			continue;
		fi

		write_line
	done < "$input_file"

	echo "Interesting syscalls have been written to '$output_file'."
}

generated_interesting_syscalls
