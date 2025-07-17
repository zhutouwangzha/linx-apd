# Field Mapper - C语言字段映射库

这是一个基于uthash库实现的高性能字段映射模块，类似于Falco的规则匹配功能。它能够将字符串字段名（如 "proc.name"、"evt.type"）与结构体偏移进行绑定，实现O(1)复杂度的字段查询。

## 功能特性

- **高性能查询**: 基于哈希表实现，查询复杂度为O(1)
- **多表支持**: 支持创建和管理多个字段映射表
- **类型安全**: 支持多种数据类型（int32, int64, uint32, uint64, string, bool, double, float）
- **嵌套字段**: 支持嵌套结构体字段访问（如 "proc.name"）
- **内存管理**: 自动管理内存分配和释放
- **易于使用**: 提供简洁的API和辅助宏

## 依赖

- uthash库（单头文件库）
- 标准C库

## 快速开始

### 1. 安装uthash

```bash
make install-uthash
```

### 2. 编译示例

```bash
make
```

### 3. 运行示例

```bash
make run
```

## API 使用说明

### 基本使用流程

```c
#include "field_mapper.h"

// 1. 初始化映射器
field_mapper_t *mapper = field_mapper_init();

// 2. 创建表
field_mapper_create_table(mapper, "event");

// 3. 添加字段映射
ADD_FIELD_MAPPING(mapper, "event", event_info_t, type, FIELD_TYPE_INT32);

// 4. 查询字段值
field_query_result_t result = field_mapper_query_value(mapper, "event", "type", &event_data);

// 5. 清理资源
field_mapper_destroy(mapper);
```

### 核心API

#### 初始化和清理

```c
// 初始化映射器
field_mapper_t* field_mapper_init(void);

// 销毁映射器
void field_mapper_destroy(field_mapper_t *mapper);
```

#### 表管理

```c
// 创建字段表
int field_mapper_create_table(field_mapper_t *mapper, const char *table_name);
```

#### 字段映射

```c
// 添加字段映射
int field_mapper_add_field(field_mapper_t *mapper, const char *table_name,
                          const char *field_name, size_t offset, 
                          field_type_t type, size_t size);

// 辅助宏：自动计算偏移和大小
ADD_FIELD_MAPPING(mapper, table, struct_type, field, type)
```

#### 查询接口

```c
// 查询字段信息
field_info_t* field_mapper_get_field_info(field_mapper_t *mapper, 
                                         const char *table_name,
                                         const char *field_name);

// 查询字段值
field_query_result_t field_mapper_query_value(field_mapper_t *mapper,
                                             const char *table_name,
                                             const char *field_name,
                                             const void *struct_ptr);
```

### 支持的数据类型

```c
typedef enum {
    FIELD_TYPE_INT32,    // 32位有符号整数
    FIELD_TYPE_INT64,    // 64位有符号整数
    FIELD_TYPE_UINT32,   // 32位无符号整数
    FIELD_TYPE_UINT64,   // 64位无符号整数
    FIELD_TYPE_STRING,   // 字符串
    FIELD_TYPE_BOOL,     // 布尔值
    FIELD_TYPE_DOUBLE,   // 双精度浮点数
    FIELD_TYPE_FLOAT     // 单精度浮点数
} field_type_t;
```

## 使用示例

### 定义数据结构

```c
typedef struct {
    int32_t pid;
    char name[256];
    bool is_container;
} proc_info_t;

typedef struct {
    int32_t type;
    uint64_t timestamp;
    proc_info_t proc;
} event_info_t;
```

### 创建映射

```c
field_mapper_t *mapper = field_mapper_init();

// 创建事件表
field_mapper_create_table(mapper, "event");

// 添加简单字段
ADD_FIELD_MAPPING(mapper, "event", event_info_t, type, FIELD_TYPE_INT32);
ADD_FIELD_MAPPING(mapper, "event", event_info_t, timestamp, FIELD_TYPE_UINT64);

// 添加嵌套字段
field_mapper_add_field(mapper, "event", "proc.pid", 
                      offsetof(event_info_t, proc) + offsetof(proc_info_t, pid),
                      FIELD_TYPE_INT32, sizeof(int32_t));

field_mapper_add_field(mapper, "event", "proc.name",
                      offsetof(event_info_t, proc) + offsetof(proc_info_t, name),
                      FIELD_TYPE_STRING, sizeof(((proc_info_t*)0)->name));
```

### 查询字段值

```c
event_info_t event_data = {
    .type = 1,
    .timestamp = 1234567890UL,
    .proc = {
        .pid = 1234,
        .name = "test_process",
        .is_container = true
    }
};

// 查询简单字段
field_query_result_t result = field_mapper_query_value(mapper, "event", "type", &event_data);
if (result.found) {
    printf("Event type: %d\n", *(int32_t*)result.value_ptr);
}

// 查询嵌套字段
result = field_mapper_query_value(mapper, "event", "proc.name", &event_data);
if (result.found) {
    printf("Process name: %s\n", (char*)result.value_ptr);
}
```

## 性能特点

- **查询复杂度**: O(1) - 基于哈希表实现
- **内存开销**: 每个字段约占用 40-60 字节（取决于字段名长度）
- **查询速度**: 在现代CPU上，单次查询通常在几纳秒到几十纳秒之间

## 应用场景

- **规则引擎**: 实现类似Falco的规则匹配系统
- **配置系统**: 动态字段访问
- **序列化/反序列化**: 基于字段名的数据处理
- **监控系统**: 高性能的事件字段提取
- **日志处理**: 结构化日志字段访问

## 注意事项

1. **内存管理**: 映射器会自动管理内存，但需要调用 `field_mapper_destroy()` 来释放资源
2. **字符串字段**: 对于字符串类型字段，返回的是指向原始数据的指针，不是拷贝
3. **线程安全**: 当前实现不是线程安全的，如需在多线程环境使用，请添加适当的同步机制
4. **字段名**: 字段名区分大小写，建议使用一致的命名规范

## 扩展功能

可以考虑添加的功能：
- 字段别名支持
- 字段验证和约束
- 序列化/反序列化支持
- 线程安全版本
- 字段监听器/回调机制

## 许可证

MIT License