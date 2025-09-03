# linx_hash_map 线程安全问题解决方案

## 问题分析

### 核心问题
在多线程环境中，`linx_hash_map_update_tables_base()` 函数存在严重的竞态条件：

1. **全局共享状态**: 所有表的 `base_addr` 存储在全局静态变量 `s_linx_hash_map` 中
2. **并发更新冲突**: 多个工作线程同时调用更新函数，相互覆盖基地址
3. **数据不一致**: 线程A设置基地址后，线程B立即覆盖，导致A获取到错误的基地址

### 问题场景
```c
// rule_match_mt.c 中的问题代码
// 线程1
linx_hash_map_update_tables_base(ctx1->tables, 5);  // 设置ctx1的基地址
// ... 匹配过程中 ...
base_addr = linx_hash_map_get_table_base("evt");     // 可能获取到ctx2的基地址！

// 线程2 同时执行
linx_hash_map_update_tables_base(ctx2->tables, 5);  // 覆盖了ctx1的基地址
```

## 解决方案

### 方案1：线程本地存储 (推荐)

**核心思路**: 每个线程维护自己的基地址副本，避免全局状态竞争

**实现要点**:
```c
// 新增线程安全API
int linx_hash_map_update_tables_base_safe(field_update_table_t *tables, size_t num_tables);
void *linx_hash_map_get_table_base_safe(const char *table_name);
void *linx_hash_map_get_value_ptr_safe(field_result_t *field, linx_field_type_t *type);
```

**优点**:
- 零锁设计，性能最优
- 完全隔离线程间的基地址
- 向后兼容，不影响单线程使用

**修改点**:
1. `rule_match_mt.c:108`: 使用 `linx_hash_map_update_tables_base_safe()`
2. `rule_match_func.c:18`: 使用 `linx_hash_map_get_value_ptr_safe()`
3. `output_match_func.c:196`: 使用 `linx_hash_map_get_value_ptr_safe()`

### 方案2：读写锁保护

**核心思路**: 使用读写锁保护全局状态，更新时独占，读取时共享

**优点**:
- 实现简单，修改量小
- 保证数据一致性

**缺点**:
- 更新时会阻塞所有读取操作
- 性能不如方案1

### 方案3：原子操作 (不推荐)

由于基地址更新涉及多个表，无法用简单原子操作解决。

## 推荐实施步骤

1. **立即修复**: 使用方案1的线程本地存储
2. **测试验证**: 在多线程环境下验证修复效果
3. **性能测试**: 确保修复不影响性能
4. **代码审查**: 检查是否还有其他类似的竞态条件

## 代码修改示例

### 修改 rule_match_mt.c
```c
// 原代码 (第107行)
linx_hash_map_update_tables_base(ctx->tables, 5);

// 修改为
linx_hash_map_update_tables_base_safe(ctx->tables, 5);
```

### 修改 rule_match_func.c
```c
// 原代码 (第17行)
void *ptr = linx_hash_map_get_value_ptr(field, type);

// 修改为
void *ptr = linx_hash_map_get_value_ptr_safe(field, type);
```

这样修改后，每个线程都会使用自己的基地址副本，彻底解决竞态条件问题。