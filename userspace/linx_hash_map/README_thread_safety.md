# linx_hash_map 线程安全解决方案

## 问题描述

在多线程环境下，当一个线程正在通过基地址+偏移的方式访问数据时，如果另一个线程同时更新了基地址，就会产生严重的竞态条件（race condition），导致：

1. **数据访问错误**：使用了无效的基地址
2. **程序崩溃**：访问到无效内存地址
3. **数据不一致**：读取到不完整或错误的数据

## 解决方案

我们实现了基于 **读写锁（pthread_rwlock_t）** 的线程安全机制：

### 核心思想

1. **读写锁保护**：每个表（`field_table_t`）都有自己的读写锁
2. **访问上下文**：提供 `access_context_t` 结构体，在锁保护期间缓存基地址
3. **原子操作**：基地址更新使用写锁，数据访问使用读锁

### 关键数据结构

```c
/**
 * 线程安全的数据访问上下文
 * 在访问数据期间保持基地址不变
 */
typedef struct {
    void *base_addr;        /* 锁定时的基地址 */
    field_table_t *table;   /* 对应的表 */
    bool locked;            /* 是否已锁定 */
} access_context_t;

/**
 * 表信息（添加了读写锁）
 */
typedef struct {
    char *table_name;
    field_info_t *fields;
    void *base_addr;        /* 结构体的基地址 */
    pthread_rwlock_t rwlock; /* 保护基地址的读写锁 */
    UT_hash_handle hh;
} field_table_t;
```

## API 设计

### 线程安全 API

#### 1. 获取访问上下文（读锁）
```c
access_context_t linx_hash_map_lock_table_read(const char *table_name);
```
- 获取指定表的读锁
- 返回包含当前基地址快照的上下文
- 多个读线程可以并发访问

#### 2. 释放访问上下文
```c
void linx_hash_map_unlock_context(access_context_t *context);
```
- 释放读锁
- 清理上下文状态

#### 3. 安全获取字段值指针
```c
static inline void *linx_hash_map_get_value_ptr_safe(const access_context_t *context, const field_result_t *field);
```
- 使用上下文中缓存的基地址
- 确保基地址在整个访问期间不变

#### 4. 原子更新基地址（写锁）
```c
int linx_hash_map_update_base_addr_safe(const char *table_name, void *new_base_addr);
```
- 获取写锁，阻塞所有读操作
- 原子性地更新基地址
- 确保更新期间没有读访问

### 兼容性 API（非线程安全）

原有的 API 保持不变，用于向后兼容：
```c
int linx_hash_map_update_base_addr(const char *table_name, void *new_base_addr);
void *linx_hash_map_get_base_addr(const char *table_name);
void *linx_hash_map_get_table_value_ptr(const char *table_name, const field_result_t *field);
```

## 使用方法

### 线程安全的数据访问

```c
// 线程安全的方式
access_context_t context = linx_hash_map_lock_table_read("student");
if (context.locked) {
    field_result_t field = linx_hash_map_get_field("student", "name");
    if (field.found) {
        char *name = (char *)linx_hash_map_get_value_ptr_safe(&context, &field);
        // 在锁保护期间，基地址不会改变
        printf("name = %s\n", name);
    }
    linx_hash_map_unlock_context(&context);
}
```

### 线程安全的基地址更新

```c
// 更新基地址
student_t *new_student = malloc(sizeof(student_t));
// ... 初始化数据 ...

// 线程安全地更新基地址
linx_hash_map_update_base_addr_safe("student", new_student);
```

## 性能特点

### 读写锁的优势

1. **并发读取**：多个线程可以同时读取数据
2. **互斥写入**：写入操作与所有读写操作互斥
3. **读者优先**：适合读多写少的场景

### 性能对比

| 操作类型 | 非线程安全 | 线程安全 | 性能影响 |
|----------|------------|----------|----------|
| 读取数据 | 直接访问 | 读锁 + 访问 | 轻微开销 |
| 更新基地址 | 直接更新 | 写锁 + 更新 | 中等开销 |
| 并发读取 | 不安全 | 支持 | 线性扩展 |

## 最佳实践

### 1. 选择合适的 API

```c
// 高并发读取场景：使用线程安全API
access_context_t context = linx_hash_map_lock_table_read("table");
// ... 数据访问 ...
linx_hash_map_unlock_context(&context);

// 单线程或已有外部同步：使用原有API
void *ptr = linx_hash_map_get_table_value_ptr("table", &field);
```

### 2. 减少锁持有时间

```c
// ✅ 好的做法：尽快释放锁
access_context_t context = linx_hash_map_lock_table_read("student");
if (context.locked) {
    field_result_t field = linx_hash_map_get_field("student", "name");
    char *name_ptr = (char *)linx_hash_map_get_value_ptr_safe(&context, &field);
    
    // 复制数据到本地缓冲区
    char local_name[32];
    strncpy(local_name, name_ptr, sizeof(local_name));
    
    linx_hash_map_unlock_context(&context);  // 尽快释放锁
    
    // 使用本地数据进行后续处理
    process_name(local_name);
}

// ❌ 不好的做法：长时间持有锁
access_context_t context = linx_hash_map_lock_table_read("student");
if (context.locked) {
    // ... 数据访问 ...
    
    // 长时间的处理逻辑
    sleep(1);  // 阻塞其他更新操作
    
    linx_hash_map_unlock_context(&context);
}
```

### 3. 错误处理

```c
access_context_t context = linx_hash_map_lock_table_read("student");
if (!context.locked) {
    fprintf(stderr, "Failed to acquire read lock\n");
    return -1;
}

// 使用 defer 模式确保锁被释放
int result = process_student_data(&context);
linx_hash_map_unlock_context(&context);
return result;
```

### 4. 避免死锁

```c
// ✅ 好的做法：固定的锁顺序
access_context_t ctx1 = linx_hash_map_lock_table_read("student");
access_context_t ctx2 = linx_hash_map_lock_table_read("teacher");
// ... 处理 ...
linx_hash_map_unlock_context(&ctx2);
linx_hash_map_unlock_context(&ctx1);

// ❌ 避免不一致的锁顺序
// 线程A：先锁student，再锁teacher
// 线程B：先锁teacher，再锁student  -> 可能死锁
```

## 编译和测试

### 编译选项

```bash
# 编译库
make clean && make

# 编译线程安全示例
make examples

# 运行线程安全演示
./build/bin/example_thread_safe
```

### 编译时需要链接线程库

```makefile
$(EXAMPLE_THREAD_SAFE): $(SRC_DIR)/example_thread_safe.c $(LIBRARY)
    @$(CC) $(CFLAGS) $< -L$(LIB_DIR) -l$(MODULE_NAME) -lpthread -o $@
```

## 示例程序说明

`example_thread_safe.c` 演示了：

1. **并发读取**：两个读线程同时访问数据
2. **动态更新**：更新线程定期更改基地址
3. **安全对比**：展示线程安全 vs 非线程安全的区别

运行示例可以观察到：
- `[安全]` 标记的操作始终获得一致的数据
- `[非安全]` 标记的操作在基地址更新期间可能看到不一致的数据

## 注意事项

1. **性能开销**：线程安全版本有一定的性能开销，但在大多数情况下是可接受的
2. **内存管理**：调用者仍需负责管理结构体实例的内存生命周期
3. **锁的粒度**：当前实现是表级锁，如需更细粒度可以考虑字段级锁
4. **不支持嵌套锁**：同一线程不应重复获取同一表的锁

## 扩展建议

1. **超时机制**：为锁操作添加超时，避免无限等待
2. **统计信息**：收集锁竞争统计，用于性能分析
3. **优先级锁**：实现写者优先或读者优先策略
4. **无锁算法**：对于特定场景，可以考虑无锁数据结构

通过这个线程安全方案，`linx_hash_map` 现在可以安全地在多线程环境中使用，有效解决了基地址更新的竞态条件问题。