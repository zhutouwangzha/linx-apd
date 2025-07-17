# Hash Map Manager - C语言字段映射库

这是一个基于uthash库实现的高性能字段映射模块，专门为类似Falco的规则匹配功能设计。它提供了一个总的hash_map管理器，可以管理多个专门的映射表（如proc表、evt表），支持自动扩容，实现O(1)复杂度的字段查询。

## 功能特性

- **总管理器设计**: 提供统一的hash_map管理器，管理多个专门的映射表
- **多表支持**: 支持创建和管理多个独立的字段映射表（如proc、evt、file等）
- **自动扩容**: 支持自动扩容机制，可配置扩容阈值
- **高性能查询**: 基于哈希表实现，查询复杂度为O(1)
- **类型安全**: 支持多种数据类型（int32/64, uint32/64, string, bool, double, float）
- **批量操作**: 支持批量添加字段映射
- **统计监控**: 提供详细的统计信息和负载因子监控
- **内存管理**: 自动管理内存分配和释放

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
make run-new      # 运行新的hash_map管理器示例
make test         # 运行单元测试
```

## 核心概念

### Hash Map Manager (总管理器)

总管理器是整个系统的核心，负责管理多个专门的映射表：

```c
hash_map_manager_t *manager = hash_map_manager_init(true, 75);  // 启用自动扩容，阈值75%
```

### 映射表 (Mapping Tables)

每个映射表专门用于特定领域的字段映射：

- **proc表**: 进程相关字段（pid, name, cmdline等）
- **evt表**: 事件相关字段（type, timestamp, direction等）
- **file表**: 文件相关字段（path, name, size等）

## API 使用说明

### 基本使用流程

```c
#include "field_mapper.h"

// 1. 初始化hash_map管理器
hash_map_manager_t *manager = hash_map_manager_init(true, 75);

// 2. 创建专门的映射表
hash_map_manager_create_table(manager, "proc", "Process information fields", 64);

// 3. 添加字段映射
ADD_FIELD_TO_TABLE(manager, "proc", proc_info_t, pid, FIELD_TYPE_INT32);

// 4. 查询字段值
field_query_result_t result = hash_map_manager_query_field(manager, "proc", "pid", &proc_data);

// 5. 清理资源
hash_map_manager_destroy(manager);
```

### 核心API

#### 管理器生命周期

```c
// 初始化管理器（自动扩容，扩容阈值）
hash_map_manager_t* hash_map_manager_init(bool auto_expand, size_t expand_threshold);

// 销毁管理器
void hash_map_manager_destroy(hash_map_manager_t *manager);
```

#### 表管理

```c
// 创建映射表
int hash_map_manager_create_table(hash_map_manager_t *manager, 
                                 const char *table_id, 
                                 const char *description,
                                 size_t initial_capacity);

// 删除映射表
int hash_map_manager_remove_table(hash_map_manager_t *manager, const char *table_id);

// 检查表是否存在
bool hash_map_manager_table_exists(hash_map_manager_t *manager, const char *table_id);
```

#### 字段映射

```c
// 添加单个字段映射
int hash_map_manager_add_field(hash_map_manager_t *manager,
                              const char *table_id,
                              const char *field_name,
                              size_t offset,
                              field_type_t type,
                              size_t size);

// 批量添加字段映射
int hash_map_manager_add_fields_batch(hash_map_manager_t *manager,
                                     const char *table_id,
                                     const field_mapping_t *mappings,
                                     size_t count);
```

#### 查询接口

```c
// 查询字段值
field_query_result_t hash_map_manager_query_field(hash_map_manager_t *manager,
                                                 const char *table_id,
                                                 const char *field_name,
                                                 const void *struct_ptr);

// 获取字段信息
field_info_t* hash_map_manager_get_field_info(hash_map_manager_t *manager,
                                             const char *table_id,
                                             const char *field_name);
```

#### 扩容管理

```c
// 手动扩容指定表
int hash_map_manager_expand_table(hash_map_manager_t *manager, 
                                 const char *table_id, 
                                 size_t new_capacity);

// 设置自动扩容参数
void hash_map_manager_set_auto_expand(hash_map_manager_t *manager, 
                                     bool enable, 
                                     size_t threshold);

// 获取表的负载因子
double hash_map_manager_get_load_factor(hash_map_manager_t *manager, const char *table_id);
```

### 辅助宏

```c
// 快速添加字段映射
#define ADD_FIELD_TO_TABLE(manager, table_id, struct_type, field, field_type) \
    hash_map_manager_add_field(manager, table_id, #field, \
                              FIELD_OFFSET(struct_type, field), \
                              field_type, sizeof(((struct_type*)0)->field))

// 获取结构体字段偏移
#define FIELD_OFFSET(struct_type, field) offsetof(struct_type, field)
```

## 使用示例

### 定义数据结构

```c
// 进程信息结构
typedef struct {
    int32_t pid;
    int32_t ppid;
    char name[256];
    char cmdline[1024];
    bool is_container;
    uint64_t start_time;
} proc_info_t;

// 事件信息结构
typedef struct {
    int32_t type;
    int32_t category;
    uint64_t timestamp;
    char direction[16];
    double duration;
} evt_info_t;
```

### 创建管理器和映射表

```c
// 初始化管理器，启用自动扩容
hash_map_manager_t *manager = hash_map_manager_init(true, 75);

// 创建proc表
hash_map_manager_create_table(manager, "proc", "Process information fields", 64);

// 创建evt表
hash_map_manager_create_table(manager, "evt", "Event information fields", 32);
```

### 添加字段映射

```c
// 方式1: 使用宏添加单个字段
ADD_FIELD_TO_TABLE(manager, "proc", proc_info_t, pid, FIELD_TYPE_INT32);
ADD_FIELD_TO_TABLE(manager, "proc", proc_info_t, name, FIELD_TYPE_STRING);

// 方式2: 批量添加字段
field_mapping_t proc_mappings[] = {
    {"pid", FIELD_OFFSET(proc_info_t, pid), FIELD_TYPE_INT32, sizeof(int32_t)},
    {"name", FIELD_OFFSET(proc_info_t, name), FIELD_TYPE_STRING, sizeof(((proc_info_t*)0)->name)},
    {"cmdline", FIELD_OFFSET(proc_info_t, cmdline), FIELD_TYPE_STRING, sizeof(((proc_info_t*)0)->cmdline)},
};
hash_map_manager_add_fields_batch(manager, "proc", proc_mappings, 3);
```

### 查询字段值

```c
proc_info_t proc_data = {
    .pid = 1234,
    .name = "test_process",
    .cmdline = "/usr/bin/test_process --arg1 --arg2",
    .is_container = true,
    .start_time = 1234567890UL
};

// 查询proc表字段
field_query_result_t result = hash_map_manager_query_field(manager, "proc", "pid", &proc_data);
if (result.found) {
    printf("Process PID: %d\n", *(int32_t*)result.value_ptr);
}

result = hash_map_manager_query_field(manager, "proc", "name", &proc_data);
if (result.found) {
    printf("Process name: %s\n", (char*)result.value_ptr);
}
```

### 监控和统计

```c
// 获取统计信息
manager_stats_t *stats = hash_map_manager_get_stats(manager);
printf("Total tables: %zu\n", stats->table_count);
printf("Total fields: %zu\n", stats->total_fields);
printf("Average load factor: %.1f%%\n", stats->avg_load_factor);

// 获取特定表的负载因子
double load_factor = hash_map_manager_get_load_factor(manager, "proc");
printf("Proc table load factor: %.1f%%\n", load_factor);

// 打印管理器信息
hash_map_manager_print_info(manager);

// 释放统计信息
hash_map_manager_free_stats(stats);
```

## 性能特点

### 时间复杂度

- **查询**: O(1) - 两次哈希查找（表查找 + 字段查找）
- **插入**: O(1) - 哈希表插入
- **删除**: O(1) - 哈希表删除

### 性能基准

基于测试结果：
- **查询速度**: ~20 纳秒/查询
- **查询吞吐**: 4600万+ 查询/秒
- **自动扩容**: 无性能影响
- **内存效率**: 线性增长，低开销

### 扩容机制

- **自动扩容**: 当负载因子超过阈值时自动扩容
- **手动扩容**: 支持手动指定新容量
- **负载监控**: 实时监控各表的负载因子

## 应用场景

### 1. 规则引擎（如Falco）

```c
// 创建专门的表来管理不同类型的字段
hash_map_manager_create_table(manager, "proc", "Process fields", 64);
hash_map_manager_create_table(manager, "evt", "Event fields", 32);
hash_map_manager_create_table(manager, "file", "File fields", 16);

// 在规则匹配中快速查询字段
field_query_result_t proc_name = hash_map_manager_query_field(manager, "proc", "name", &event.proc);
field_query_result_t evt_type = hash_map_manager_query_field(manager, "evt", "type", &event);
```

### 2. 配置系统

```c
// 不同模块的配置字段分别管理
hash_map_manager_create_table(manager, "database", "Database config", 16);
hash_map_manager_create_table(manager, "network", "Network config", 16);
hash_map_manager_create_table(manager, "security", "Security config", 16);
```

### 3. 监控系统

```c
// 分类管理监控指标
hash_map_manager_create_table(manager, "cpu", "CPU metrics", 32);
hash_map_manager_create_table(manager, "memory", "Memory metrics", 32);
hash_map_manager_create_table(manager, "disk", "Disk metrics", 32);
```

## 错误处理

模块使用明确的错误码：

```c
#define HASH_MAP_SUCCESS            0
#define HASH_MAP_ERROR_NULL_PARAM   -1
#define HASH_MAP_ERROR_TABLE_EXISTS -2
#define HASH_MAP_ERROR_TABLE_NOT_FOUND -3
#define HASH_MAP_ERROR_FIELD_EXISTS -4
#define HASH_MAP_ERROR_FIELD_NOT_FOUND -5
#define HASH_MAP_ERROR_MEMORY       -6
#define HASH_MAP_ERROR_INVALID_PARAM -7
```

## 注意事项

1. **内存管理**: 管理器会自动管理内存，但需要调用 `hash_map_manager_destroy()` 来释放资源
2. **字符串字段**: 对于字符串类型字段，返回的是指向原始数据的指针，不是拷贝
3. **线程安全**: 当前实现不是线程安全的，如需在多线程环境使用，请添加适当的同步机制
4. **表命名**: 表ID区分大小写，建议使用一致的命名规范
5. **自动扩容**: 扩容只增加容量标记，不重新分配哈希表（uthash自动管理）

## 编译和运行

```bash
# 编译所有目标
make

# 运行新的示例
make run-new

# 运行单元测试
make test

# 清理编译文件
make clean
```

## 许可证

MIT License