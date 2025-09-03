# linx-apd 多线程安全解决方案

## 问题描述

原始的 linx-apd 采用单线程方式进行规则匹配，在多线程环境下会出现以下问题：

1. **全局共享状态冲突**：`linx_hash_map` 使用全局静态变量存储表信息
2. **base_addr 并发更新**：每次事件处理时会更新各个表的 `base_addr`，多线程同时访问会导致竞态条件
3. **数据一致性问题**：线程间可能读取到不一致的 `base_addr` 值

## 解决方案

采用 **线程本地存储 (Thread-Local Storage) + 读写锁** 的混合方案：

### 核心思想

1. **线程本地存储**：每个线程维护自己的 `base_addr` 副本
2. **字段定义共享**：字段结构定义在线程间共享（只读）
3. **线程注册机制**：提供线程注册/注销管理

### 架构设计

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Thread 1      │    │   Thread 2      │    │   Thread N      │
│                 │    │                 │    │                 │
│ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │Thread Local │ │    │ │Thread Local │ │    │ │Thread Local │ │
│ │ Hash Map    │ │    │ │ Hash Map    │ │    │ │ Hash Map    │ │
│ │             │ │    │ │             │ │    │ │             │ │
│ │base_addr_1  │ │    │ │base_addr_2  │ │    │ │base_addr_N  │ │
│ │base_addr_2  │ │    │ │base_addr_2  │ │    │ │base_addr_2  │ │
│ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────┘ │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                    ┌─────────────────┐
                    │   Global        │
                    │   Hash Map      │
                    │                 │
                    │ Field Defs      │
                    │ (Read-Only)     │
                    └─────────────────┘
```

## 实现细节

### 1. 线程安全的 Hash Map

- **文件**: `linx_hash_map_thread_safe.c/h`
- **关键结构**:
  ```c
  typedef struct {
      thread_local_field_table_t *tables;
      size_t size;
      size_t capacity;
  } thread_local_linx_hash_map_t;
  ```

### 2. 线程本地存储管理

- 使用 `pthread_key_t` 实现线程本地存储
- 自动清理机制，线程结束时释放资源
- 线程注册/注销API

### 3. 事件处理修改

- **文件**: `linx_event_rich_thread_safe.c/h`
- 每个线程维护独立的 `event_t` 结构
- 使用 `__thread` 关键字实现线程本地变量

### 4. 规则匹配修改

- **文件**: `linx_rule_engine_set_thread_safe.c/h`
- 线程安全的规则匹配函数
- 使用线程本地的字段查询API

## 使用方法

### 1. 初始化

```c
// 初始化线程安全hash map
int ret = linx_hash_map_thread_safe_init();
if (ret != 0) {
    // 处理错误
}
```

### 2. 线程注册

```c
// 在工作线程中注册
ret = linx_hash_map_register_thread();
if (ret != 0) {
    // 处理错误
}
```

### 3. 事件处理

```c
// 使用线程安全的事件处理
ret = linx_event_rich_thread_safe(event);
if (ret != 0) {
    // 处理错误
}

// 使用线程安全的规则匹配
bool matched = linx_rule_set_match_rule_thread_safe();
```

### 4. 线程清理

```c
// 线程结束时清理
linx_event_rich_thread_cleanup();
linx_hash_map_unregister_thread();
```

### 5. 全局清理

```c
// 程序结束时清理
linx_hash_map_thread_safe_deinit();
```

## API 参考

### 线程管理

```c
int linx_hash_map_thread_safe_init(void);
void linx_hash_map_thread_safe_deinit(void);
int linx_hash_map_register_thread(void);
void linx_hash_map_unregister_thread(void);
```

### 线程本地操作

```c
int linx_hash_map_thread_local_update_table_base(const char *table_name, void *base_addr);
int linx_hash_map_thread_local_update_tables_base(field_update_table_t *tables, size_t num_tables);
void *linx_hash_map_thread_local_get_table_base(const char *table_name);
field_result_t linx_hash_map_thread_local_get_field(const char *table_name, const char *field_name);
```

### 事件处理

```c
int linx_event_rich_thread_safe(linx_event_t *event);
event_t *linx_event_rich_get_thread_safe(void);
void linx_event_rich_thread_cleanup(void);
```

### 规则匹配

```c
bool linx_rule_set_match_rule_thread_safe(void);
int linx_event_process_thread_safe(linx_event_t *event);
void linx_rule_engine_thread_cleanup(void);
```

## 编译

### 编译线程安全库

```bash
cd userspace/linx_hash_map
make -f Makefile.thread_safe
```

### 链接选项

```bash
gcc -o your_program your_program.c -llinx_hash_map_thread_safe -lpthread
```

## 测试

运行测试程序验证多线程安全性：

```bash
cd test
gcc -o test_thread_safe test_thread_safe_hash_map.c -I../userspace/linx_hash_map/include -L../userspace/linx_hash_map -llinx_hash_map_thread_safe -lpthread
./test_thread_safe
```

## 性能考虑

### 优势
- **无锁访问**：线程本地存储避免了锁竞争
- **高并发**：支持多线程同时处理事件
- **内存隔离**：线程间不会相互干扰

### 开销
- **内存开销**：每个线程需要额外的存储空间
- **初始化成本**：线程注册时需要复制表结构

### 优化建议
1. **线程池**：使用固定大小的线程池减少线程创建开销
2. **预分配**：预先分配线程本地存储空间
3. **批处理**：批量更新多个表的base_addr

## 迁移指南

### 从单线程迁移到多线程

1. **替换头文件**:
   ```c
   // 原有
   #include "linx_hash_map.h"
   #include "linx_event_rich.h"
   
   // 新增
   #include "linx_hash_map_thread_safe.h"
   #include "linx_event_rich_thread_safe.h"
   ```

2. **替换函数调用**:
   ```c
   // 原有
   linx_hash_map_update_tables_base(tables, count);
   linx_event_rich(event);
   linx_rule_set_match_rule();
   
   // 替换为
   linx_hash_map_thread_local_update_tables_base(tables, count);
   linx_event_rich_thread_safe(event);
   linx_rule_set_match_rule_thread_safe();
   ```

3. **添加线程管理**:
   ```c
   // 线程开始
   linx_hash_map_register_thread();
   
   // 线程结束
   linx_event_rich_thread_cleanup();
   linx_hash_map_unregister_thread();
   ```

## 故障排除

### 常见问题

1. **线程未注册**
   - 错误：`Failed to register thread`
   - 解决：确保在使用前调用 `linx_hash_map_register_thread()`

2. **字段未找到**
   - 错误：`Field not found`
   - 解决：确保全局hash map已正确初始化并添加了字段定义

3. **内存泄漏**
   - 问题：线程结束后内存未释放
   - 解决：确保调用清理函数

### 调试技巧

1. **启用日志**：设置 `LINX_LOG_LEVEL=DEBUG`
2. **检查线程ID**：使用 `pthread_self()` 确认线程身份
3. **内存检查**：使用 valgrind 检测内存问题

## 总结

该解决方案通过线程本地存储有效解决了 linx-apd 在多线程环境下的竞态条件问题，同时保持了良好的性能和可维护性。实现了：

- ✅ 线程安全的 base_addr 管理
- ✅ 高性能的并发访问
- ✅ 完整的资源管理
- ✅ 向后兼容性
- ✅ 详细的测试和文档