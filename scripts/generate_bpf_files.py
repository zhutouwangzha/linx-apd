#!/usr/bin/env python3

import re
import os
import argparse
import sys

# 正则表达式用于解析头文件中的定义
pattern = re.compile(
    r'SYSCALL_MACRO\((\w+),\s*(\w+),\s*(\d+),\s*(\d+),\s*(\d+)\)\s*'
    r'ENTER_PARAM_MACRO\(([^)]*)\)\s*'
    r'EXIT_PARAM_MACRO\(([^)]*)\)'
)

def generate_bpf_file(syscall_upper, syscall_lower, enter_params):
    # 类型映射表：C类型 --> （存储函数，转换类型）
    type_map = {
        # int8_t
        "char": ("linx_ringbuf_store_s8", "int8_t"),

        # int16_t
        "short": ("linx_ringbuf_store_s16", "int16_t"),

        # int32_t
        "int": ("linx_ringbuf_store_s32", "int32_t"),
        "int *": ("linx_ringbuf_store_s32", "int32_t *"),
        "__s32": ("linx_ringbuf_store_s32", "int32_t"),
        "key_t": ("linx_ringbuf_store_s32", "int32_t"),
        "pid_t": ("linx_ringbuf_store_s32", "int32_t"),
        "timer_t": ("linx_ringbuf_store_s32", "int32_t"),
        "clockid_t": ("linx_ringbuf_store_s32", "int32_t"),
        "const clockid_t": ("linx_ringbuf_store_s32", "int32_t"),
        "mqd_t": ("linx_ringbuf_store_s32", "int32_t"),
        "key_serial_t": ("linx_ringbuf_store_s32", "int32_t"),
        "rwf_t": ("linx_ringbuf_store_s32", "int32_t"),

        # int64_t
        "long": ("linx_ringbuf_store_s64", "int64_t"),
        "off_t": ("linx_ringbuf_store_s64", "int64_t"),
        "loff_t": ("linx_ringbuf_store_s64", "int64_t"),
        "loff_t *": ("linx_ringbuf_store_s64", "int64_t *"),

        # uint8_t
        "unsigned char": ("linx_ringbuf_store_u8", "uint8_t"),
    
        # uint16_t
        "unsigned short": ("linx_ringbuf_store_u16", "uint16_t"),
        "umode_t": ("linx_ringbuf_store_u16", "uint16_t"),

        # uint32_t
        "unsigned": ("linx_ringbuf_store_u32", "uint32_t"),
        "unsigned int": ("linx_ringbuf_store_u32", "uint32_t"),
        "unsigned *": ("linx_ringbuf_store_u32", "uint32_t *"),
        "unsigned int *": ("linx_ringbuf_store_u32", "uint32_t *"),
        "u32": ("linx_ringbuf_store_u32", "uint32_t"),
        "u32 *": ("linx_ringbuf_store_u32", "uint32_t *"),
        "uid_t": ("linx_ringbuf_store_u32", "uint32_t"),
        "uid_t *": ("linx_ringbuf_store_u32", "uint32_t *"),
        "gid_t": ("linx_ringbuf_store_u32", "uint32_t"),
        "gid_t *": ("linx_ringbuf_store_u32", "uint32_t *"),
        "qid_t": ("linx_ringbuf_store_u32", "uint32_t"),
        
        # uint64_t
        "unsigned long": ("linx_ringbuf_store_u64", "uint64_t"),
        "unsigned long *": ("linx_ringbuf_store_u64", "uint64_t *"),
        "const unsigned long *": ("linx_ringbuf_store_u64", "uint64_t *"),
        "size_t": ("linx_ringbuf_store_u64", "uint64_t"),
        "size_t *": ("linx_ringbuf_store_u64", "uint64_t *"),
        "aio_context_t": ("linx_ringbuf_store_u64", "uint64_t"),
        "char *": ("linx_ringbuf_store_charpointer", "uint64_t"),
        "unsigned char *": ("linx_ringbuf_store_charpointer", "uint64_t"),
        "const char *": ("linx_ringbuf_store_charpointer", "uint64_t"),
        "const unsigned char *": ("linx_ringbuf_store_charpointer", "uint64_t"),
        "const char *const *": ("linx_ringbuf_store_charpointer", "uint64_t"),
    }

    # 生成参数存储代码
    store_code = []
    for i, (ptype, pname) in enumerate(enter_params):
        # 生成参数注释
        store_code.append(f"    /* {ptype} {pname} */")
    
        # 查找类型映射
        store_func, cast_type = type_map.get(
            ptype,                                  # 精确查找
            ("linx_ringbuf_store_u64", "uint64_t")  # 未匹配的默认处理
        )

        # 处理字符串指针类型
        if store_func == "linx_ringbuf_store_charpointer":
            store_code.append(f"    {cast_type} __{pname} = ({cast_type})get_pt_regs_argumnet(regs, {i});")
            store_code.append(f"    linx_ringbuf_store_charpointer(ringbuf, __{pname}, LINX_CHARBUF_MAX_SIZE, USER);")
        # 处理当前支持的指针类型
        elif '*' in cast_type:
            clean_cast_type = cast_type.replace(' *', '')
            store_code.append(f"    {cast_type}__{pname} = ({cast_type})get_pt_regs_argumnet(regs, {i});")
            store_code.append(f"    {clean_cast_type} ___{pname} = 0;")
            store_code.append(f"    if (__{pname}) {{ ")
            store_code.append(f"        bpf_probe_read_user(&___{pname}, sizeof(___{pname}), __{pname});")
            store_code.append(f"    }}")
            store_code.append(f"    {store_func}(ringbuf, ___{pname});")
        # 处理当前不支持的指针类型(默认都采集地址)
        elif '*' in ptype:
            store_code.append(f"    uint64_t __{pname} = (uint64_t)get_pt_regs_argumnet(regs, {i});")
            store_code.append(f"    linx_ringbuf_store_u64(ringbuf, __{pname});")
        # 处理基本类型
        else:
            store_code.append(f"    {cast_type} __{pname} = ({cast_type})get_pt_regs_argumnet(regs, {i});")
            store_code.append(f"    {store_func}(ringbuf, __{pname});")
        
        # 在参数之间添加空行
        store_code.append(f"")

    store_code = "\n".join(store_code)

    # 文件内容模板
    return f"""#include "get_pt_regs.h"
#include "ringbuf_func.h"

SEC("tp_btf/sys_enter")
int BPF_PROG({syscall_lower}_e, struct pt_regs *regs, long id)
{{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {{
        return 0;
    }}

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_{syscall_upper}_E, -1);

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}}

SEC("tp_btf/sys_exit")
int BPF_PROG({syscall_lower}_x, struct pt_regs *regs, long ret)
{{
    linx_ringbuf_t *ringbuf = linx_ringbuf_get();
    if (!ringbuf) {{
        return 0;
    }}

    linx_ringbuf_load_event(ringbuf, LINX_EVENT_TYPE_{syscall_upper}_X, ret);

{store_code}

    linx_ringbuf_submit_event(ringbuf);

    return 0;
}}
"""

def pares_enter_params(param_str):
    # 移除多余空格并分割参数
    items = [item.strip() for item in param_str.split(",")]
    params = []

    # 成对处理类型和名称
    for i in range(0, len(items), 2):
        if i + 1 >= len(items):
            break
        
        ptype = items[i]
        pname = items[i + 1]
        params.append((ptype, pname))

    return params

def process_header_file(input_path, output_dir):
    # 确保输出目录存在
    os.makedirs(output_dir, exist_ok=True)

    # 读取头文件内容
    try:
        with open(input_path, 'r') as f:
            header_content = f.read()
    except FileNotFoundError:
        print(f"错误：输入文件不存在 {input_path}")
        sys.exit(1)
    
    # 处理每一行
    generate_count = 0
    for line in header_content.splitlines():
        match = pattern.search(line)
        if not match:
            continue
        
        # 解析宏的参数
        syscall_upper = match.group(1)
        syscall_lower = match.group(2)
        syscall_number = int(match.group(3))
        enter_params = pares_enter_params(match.group(6))

        # 生成BPF文件
        bpf_content = generate_bpf_file(syscall_upper, syscall_lower, enter_params)
        filename = os.path.join(output_dir, f"{syscall_number:03d}-{syscall_lower}.bpf.c")

        with open(filename, "w") as f:
            f.write(bpf_content)
        print(f"已生成：{filename}")
        generate_count += 1
    
    print(f"\n生成成功 {generate_count} 个BPF文件到目录：{output_dir}")

def main():
    parser = argparse.ArgumentParser(
        description='从linx_syscall_macro.h文件生成系统调用BPF采集文件')
    parser.add_argument('-i', '--input', required=True,
                        help='输入头文件路径(linx_syscall_macro.h)')
    parser.add_argument('-o', '--output', required=True,
                        help='输出目录路径')

    args = parser.parse_args()

    if not os.path.isfile(args.input):
        print(f"错误：输入文件不存在 {args.input}")
        sys.exit(1)

    process_header_file(args.input, args.output)

if __name__ == "__main__":
    main()
