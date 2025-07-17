# 规则输出功能实现文档

## 概述

本文档描述了如何在falco-like工具中实现规则匹配成功时的输出功能。当规则匹配成功时，系统会将yaml文件中的`output`字段进行变量替换并输出。

## 实现的功能

### 1. 变量替换支持

支持以下变量的自动替换：
- `%evt.time` - 当前时间戳
- `%user.name` - 当前用户名
- `%proc.name` - 进程名称
- `%proc.pid` - 进程ID
- `%proc.ppid` - 父进程ID
- `%proc.cmdline` - 进程命令行
- `%fd.name` - 文件名（临时使用固定值）
- `%evt.arg.data` - 事件参数数据（临时使用固定值）

### 2. 输出格式化

将yaml规则文件中的output模板字符串转换为实际的输出内容。

## 文件结构

```
userspace/linx_rule_engine/rule_engine_output/
├── include/
│   └── linx_rule_output.h          # 头文件
├── linx_rule_output.c              # 实现文件
├── test_output.c                   # 测试程序
└── Makefile                        # 编译配置
```

## 核心函数

### `linx_rule_output_format_and_print()`

```c
int linx_rule_output_format_and_print(const linx_rule_t *rule);
```

- **功能**: 格式化并输出规则匹配结果
- **参数**: `rule` - 匹配的规则结构体指针
- **返回值**: 0表示成功，-1表示失败
- **说明**: 将规则的output字段中的变量替换为实际值并输出到stdout

### `linx_rule_output_format_string()`

```c
int linx_rule_output_format_string(const char *output_template, char **formatted_output);
```

- **功能**: 格式化输出字符串
- **参数**: 
  - `output_template` - 输出模板字符串
  - `formatted_output` - 格式化后的输出字符串（需要调用者释放）
- **返回值**: 0表示成功，-1表示失败

## 使用方法

### 1. 在规则集合中集成

修改 `userspace/linx_rule_engine/rule_engine_set/linx_rule_engine_set.c` 文件：

```c
#include "linx_rule_output.h"

bool linx_rule_set_match_rule(void)
{
    bool match = false;

    if (rule_set == NULL) {
        return false;
    }

    for (size_t i = 0; i < rule_set->size; i++) {
        if (rule_set->data.matches[i]) {
            if (rule_set->data.matches[i]->func(rule_set->data.matches[i]->context)) {
                match = true;
                
                // 规则匹配成功，格式化并输出yaml中的Output内容
                if (rule_set->data.rules[i]) {
                    linx_rule_output_format_and_print(rule_set->data.rules[i]);
                }

                break;
            }
        }
    }

    return match;
}
```

### 2. 编译配置

在主Makefile中添加输出模块的include路径：

```makefile
INCLUDE := ... \
           -I$(USR_DIR)/linx_rule_engine/rule_engine_output/include \
           ...
```

### 3. 链接配置

确保在链接时包含输出模块的库文件。

## 示例输出

### 原始yaml规则文件
```yaml
- rule: Passwd File Access by File Open
  desc: Detect direct file open of sensitive files by known read tools
  condition: >
    evt.type = execve and
    proc.name in (vim, vi, nvim, cat, less, more, tail, head) and
    proc.args = "/etc/passwd" and
    proc.cmdline contains "/etc/passwd"
  output: >
    [ALERT] %evt.time User=%user.name opened sensitive file %fd.name using %proc.name (%proc.cmdline pid=%proc.pid ppid=%proc.ppid)
  priority: WARNING
  tags: [filesystem, sensitive]
```

### 格式化后的输出
```
[ALERT] 2025-07-17 06:44:43.573 User=ubuntu opened sensitive file /etc/passwd using test_output (./test_output  pid=3194 ppid=2765)
```

## 测试

运行测试程序验证功能：

```bash
cd userspace/linx_rule_engine/rule_engine_output
make
gcc -Wall -Wextra -std=c99 -g -O2 -I./include -I../rule_engine_load/include test_output.c -L. -llinx_rule_output -o test_output
./test_output
```

## 扩展功能

### 1. 动态字段获取

当前实现中，某些字段（如`%fd.name`、`%evt.arg.data`）使用固定值。在实际应用中，这些值应该从事件数据中动态获取。

### 2. 更多变量支持

可以根据需要添加更多变量支持，如：
- `%evt.type` - 事件类型
- `%evt.dir` - 事件方向
- `%fd.type` - 文件类型
- `%net.proto` - 网络协议
- 等等

### 3. 输出目标配置

可以扩展输出功能，支持输出到不同目标：
- 标准输出（当前实现）
- 日志文件
- 网络接口
- 数据库

## 注意事项

1. 内存管理：使用`linx_rule_output_format_string()`函数时，需要手动释放返回的字符串
2. 线程安全：当前实现不是线程安全的，如果在多线程环境中使用，需要添加适当的同步机制
3. 错误处理：函数会返回错误码，调用者应该检查返回值并适当处理错误情况

## 总结

通过这个实现，你的falco-like工具现在可以：
1. 解析yaml规则文件中的output字段
2. 在规则匹配成功时进行变量替换
3. 输出格式化后的告警信息

这为你的安全监控工具提供了完整的告警输出功能，符合falco的设计理念。