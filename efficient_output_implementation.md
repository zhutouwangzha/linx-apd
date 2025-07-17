# 高效规则输出系统实现文档

## 概述

本文档描述了基于结构体偏移绑定的高效规则输出系统。该系统解决了每次规则匹配都需要进行字符串替换的性能问题，通过预编译输出模板和字段偏移绑定，实现了高性能的规则输出功能。

## 核心设计思想

### 1. 预编译输出模板

- **问题**: 每次规则匹配都进行字符串替换效率低下
- **解决方案**: 在规则加载时预编译输出模板，将模板解析为字面量片段和变量引用
- **优势**: 运行时只需要简单的内存拷贝和字段访问，无需字符串搜索和替换

### 2. 结构体偏移绑定

- **问题**: 动态字段查找和类型转换开销大
- **解决方案**: 使用编译时确定的结构体偏移量直接访问字段
- **优势**: O(1)的字段访问速度，无需哈希表查找或字符串比较

### 3. 分离的输出匹配结构

- **设计**: 创建独立的`linx_output_match_t`结构体
- **优势**: 不影响现有的`linx_rule_match_t`结构体，易于集成和维护

## 架构设计

### 核心结构体

```c
// 输出匹配结构体
typedef struct {
    output_segment_t *segments;        // 预解析的输出片段
    field_binding_t *bindings;         // 字段绑定信息
    size_t segment_count;              // 片段数量
    size_t binding_count;              // 绑定数量
    size_t estimated_output_size;      // 预估输出大小
} linx_output_match_t;

// 事件数据结构体
typedef struct {
    struct linx_timespec timestamp;    // %evt.time
    pid_t pid;                         // %proc.pid
    pid_t ppid;                        // %proc.ppid
    char *process_name;                // %proc.name
    char *username;                    // %user.name
    char *file_name;                   // %fd.name
    // ... 更多字段
} linx_event_data_t;

// 字段绑定信息
typedef struct {
    const char *variable_name;         // 变量名，如"%evt.time"
    field_type_t field_type;           // 字段类型
    size_t offset;                     // 结构体偏移量
    size_t size;                       // 字段大小
} field_binding_t;
```

### 工作流程

1. **规则加载时**:
   - 解析yaml中的output字段
   - 创建`linx_output_match_t`结构体
   - 注册字段绑定（变量名 -> 结构体偏移）
   - 编译输出模板（解析为片段数组）

2. **规则匹配时**:
   - 使用预编译的输出匹配结构
   - 通过偏移量直接访问事件数据字段
   - 快速生成格式化输出

## 实现细节

### 1. 字段类型支持

```c
typedef enum {
    FIELD_TYPE_STRING,      // 字符串类型
    FIELD_TYPE_INT32,       // 32位整数
    FIELD_TYPE_INT64,       // 64位整数
    FIELD_TYPE_UINT32,      // 32位无符号整数
    FIELD_TYPE_UINT64,      // 64位无符号整数
    FIELD_TYPE_UINT16,      // 16位无符号整数
    FIELD_TYPE_TIMESTAMP,   // 时间戳类型
    FIELD_TYPE_CUSTOM,      // 自定义处理函数
} field_type_t;
```

### 2. 输出片段类型

```c
typedef enum {
    SEGMENT_TYPE_LITERAL,   // 字面量文本
    SEGMENT_TYPE_VARIABLE   // 变量引用
} segment_type_t;
```

### 3. 模板编译过程

```c
// 输入: "[ALERT] %evt.time User=%user.name opened %fd.name"
// 输出片段:
// 1. LITERAL: "[ALERT] "
// 2. VARIABLE: %evt.time (绑定到timestamp字段)
// 3. LITERAL: " User="
// 4. VARIABLE: %user.name (绑定到username字段)
// 5. LITERAL: " opened "
// 6. VARIABLE: %fd.name (绑定到file_name字段)
```

## 性能优势

### 1. 时间复杂度对比

| 操作 | 传统方法 | 高效方法 |
|------|----------|----------|
| 字段查找 | O(n) 字符串搜索 | O(1) 偏移访问 |
| 变量替换 | O(n*m) 字符串替换 | O(1) 内存拷贝 |
| 输出生成 | O(n*m*k) 每次解析 | O(n) 线性拷贝 |

### 2. 空间复杂度优化

- **预编译**: 一次性内存分配，避免重复解析
- **预估大小**: 提前计算输出缓冲区大小，减少内存重分配
- **段缓存**: 字面量文本只存储一次

### 3. 实际性能测试

```c
// 测试结果示例（10000次迭代）
Pre-compiled method: 10000 iterations in 0.0234 seconds (427350.43 ops/sec)
```

## 使用方法

### 1. 基本使用

```c
// 创建输出匹配
linx_output_match_t *output_match = linx_output_match_create();

// 注册字段绑定
linx_output_integration_register_default_bindings(output_match);

// 编译输出模板
linx_output_match_compile(output_match, rule->output);

// 创建事件数据
linx_event_data_t *event_data = linx_event_data_create();
linx_event_data_set_string(event_data, "username", "admin");
linx_event_data_set_string(event_data, "file_name", "/etc/passwd");

// 快速生成输出
linx_output_match_format_and_print(output_match, event_data);
```

### 2. 与现有系统集成

```c
// 在规则加载时为每个规则创建输出匹配
for (size_t i = 0; i < rule_count; i++) {
    linx_output_integration_create_match_for_rule(&rules[i], &output_matches[i]);
}

// 在规则匹配成功时使用预编译的输出匹配
if (rule_matched) {
    linx_output_match_format_and_print(output_matches[rule_index], event_data);
}
```

### 3. 自定义字段处理

```c
// 自定义处理函数
static int handle_custom_field(const void *data, char *output, size_t output_size) {
    // 自定义逻辑
    snprintf(output, output_size, "custom_value");
    return 0;
}

// 注册自定义绑定
linx_output_match_add_custom_binding(output_match, "%custom.field", handle_custom_field);
```

## 集成指南

### 1. 修改规则集合结构

```c
// 在现有的规则集合中添加输出匹配数组
typedef struct {
    linx_rule_t **rules;
    linx_rule_match_t **matches;
    linx_output_match_t **output_matches;  // 新增
    size_t size;
    size_t capacity;
} linx_rule_set_t;
```

### 2. 修改规则加载流程

```c
// 在规则加载时创建输出匹配
static int linx_rule_engine_add_rule_to_set(linx_yaml_node_t *root) {
    // ... 现有的规则解析代码 ...
    
    // 创建输出匹配
    linx_output_match_t *output_match = NULL;
    if (linx_output_integration_create_match_for_rule(rule, &output_match) == 0) {
        // 添加到规则集合
        rule_set->output_matches[rule_set->size] = output_match;
    }
    
    // ... 其余代码 ...
}
```

### 3. 修改规则匹配流程

```c
// 在规则匹配成功时使用高效输出
bool linx_rule_set_match_rule(void) {
    // ... 现有的匹配逻辑 ...
    
    if (rule_matched) {
        // 使用预编译的输出匹配
        if (rule_set->output_matches[i]) {
            linx_output_match_format_and_print(rule_set->output_matches[i], 
                                               current_event_data);
        }
    }
    
    // ... 其余代码 ...
}
```

## 支持的字段变量

### 时间字段
- `%evt.time` - 事件时间戳

### 进程字段
- `%proc.pid` - 进程ID
- `%proc.ppid` - 父进程ID
- `%proc.name` - 进程名称
- `%proc.cmdline` - 进程命令行

### 用户字段
- `%user.name` - 用户名
- `%user.uid` - 用户ID

### 文件字段
- `%fd.name` - 文件名
- `%fd.path` - 文件路径
- `%fd.type` - 文件类型

### 事件字段
- `%evt.type` - 事件类型
- `%evt.dir` - 事件方向
- `%evt.arg.data` - 事件参数数据
- `%evt.res` - 事件结果

### 网络字段
- `%net.src_ip` - 源IP地址
- `%net.dst_ip` - 目标IP地址
- `%net.src_port` - 源端口
- `%net.dst_port` - 目标端口
- `%net.proto` - 网络协议

### 容器字段
- `%container.id` - 容器ID
- `%container.name` - 容器名称

## 扩展功能

### 1. 添加新字段类型

```c
// 1. 在field_type_t枚举中添加新类型
typedef enum {
    // ... 现有类型 ...
    FIELD_TYPE_FLOAT,       // 浮点数类型
    FIELD_TYPE_BOOLEAN,     // 布尔类型
} field_type_t;

// 2. 在format_field_value函数中添加处理逻辑
case FIELD_TYPE_FLOAT: {
    float float_val = *(const float*)data_ptr;
    snprintf(output, output_size, "%.2f", float_val);
    break;
}
```

### 2. 添加新的输出目标

```c
// 扩展输出函数，支持多种输出目标
typedef enum {
    OUTPUT_TARGET_STDOUT,
    OUTPUT_TARGET_FILE,
    OUTPUT_TARGET_SYSLOG,
    OUTPUT_TARGET_NETWORK,
} output_target_t;

int linx_output_match_format_and_output(const linx_output_match_t *output_match,
                                        const void *data_struct,
                                        output_target_t target,
                                        void *target_config);
```

### 3. 条件输出

```c
// 支持条件输出，只有满足条件时才输出
typedef struct {
    linx_output_match_t *output_match;
    bool (*condition)(const void *data_struct);
} conditional_output_t;
```

## 注意事项

### 1. 内存管理
- 使用`linx_output_match_create()`和`linx_output_match_destroy()`管理输出匹配生命周期
- 事件数据结构体的字符串字段需要手动管理内存
- 格式化输出的缓冲区由调用者负责管理

### 2. 线程安全
- 当前实现不是线程安全的
- 在多线程环境中使用时需要适当的同步机制
- 事件数据结构体应该是线程本地的

### 3. 错误处理
- 所有函数都返回错误码，调用者应该检查返回值
- 字段绑定失败时会跳过该变量
- 输出缓冲区溢出会被截断

### 4. 性能考虑
- 预编译的输出匹配应该在规则加载时创建，而不是每次匹配时创建
- 事件数据结构体的字段更新应该批量进行
- 对于高频匹配的规则，可以考虑使用对象池来管理事件数据

## 总结

这个高效的规则输出系统通过以下技术实现了显著的性能提升：

1. **预编译模板**: 避免运行时字符串解析
2. **偏移绑定**: 实现O(1)的字段访问
3. **分离设计**: 不影响现有架构
4. **类型安全**: 支持多种字段类型
5. **扩展性**: 支持自定义字段处理

该系统特别适合高并发的安全监控场景，能够在保持功能完整性的同时显著提升性能。