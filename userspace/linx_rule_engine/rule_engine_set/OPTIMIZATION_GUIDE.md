# LINX 规则集合优化方案

## 概述

本优化方案通过实现**事件源-事件方向-事件类型**三级分类系统，显著提高了规则匹配的效率。当事件到来时，系统首先对事件进行分类，然后只匹配相关的规则子集，从而避免了不必要的规则匹配。

## 核心设计

### 1. 三级分类体系

#### 第一级：事件源 (Event Source)
根据系统调用的功能领域进行分类：
- `FILE_IO`: 文件I/O操作 (read, write, open, close等)
- `NETWORK`: 网络操作 (socket, connect, sendto等)
- `PROCESS`: 进程管理 (fork, exec, clone等)
- `MEMORY`: 内存管理 (mmap, munmap, brk等)
- `SIGNAL`: 信号处理 (sigaction, kill等)
- `IPC`: 进程间通信 (pipe, msgget, semget等)
- `FILESYSTEM`: 文件系统操作 (stat, chmod, mkdir等)
- `SECURITY`: 安全相关 (setuid, capget等)
- `SYSTEM`: 系统信息 (uname, sysinfo等)
- `TIME`: 时间相关 (gettimeofday, nanosleep等)

#### 第二级：事件方向 (Event Direction)
- `ENTER`: 系统调用进入 (事件类型为偶数)
- `EXIT`: 系统调用退出 (事件类型为奇数)

#### 第三级：事件类型 (Event Type)
具体的系统调用类型，如 `LINX_EVENT_TYPE_OPEN_E`、`LINX_EVENT_TYPE_READ_X` 等

### 2. 数据结构优化

#### 原始结构
```c
typedef struct {
    struct {
        linx_rule_t **rules;
        linx_rule_match_t **matches;
        linx_output_match_t **outputs;
    } data;
    size_t size;
    size_t capacity;
} linx_rule_set_t;
```

#### 优化后结构
```c
typedef struct {
    /* 三级索引: [source][direction][type] */
    linx_rule_list_t ***indexed_rules;
    
    /* 通用规则列表 - 不依赖特定事件分类的规则 */
    linx_rule_list_t *generic_rules;
    
    /* 统计信息 */
    struct {
        size_t total_rules;
        size_t indexed_rules;
        size_t generic_rules;
        size_t match_attempts;
        size_t successful_matches;
        size_t skipped_rules;  /* 通过分类过滤跳过的规则数 */
    } stats;
    
    bool initialized;
} linx_rule_set_t;
```

### 3. 核心模块

#### 事件分类器 (Event Classifier)
- **文件**: `linx_event_classifier.h/c`
- **功能**: 将事件按照三级体系进行分类
- **核心函数**: `linx_event_classify()`

#### 规则分析器 (Rule Analyzer)
- **文件**: `linx_rule_analyzer.h/c`
- **功能**: 分析规则条件，提取适用的事件分类信息
- **核心函数**: `linx_rule_analyze()`

#### 优化后的规则集合 (Rule Set)
- **文件**: `linx_rule_engine_set.h/c`
- **功能**: 基于三级索引的规则存储和匹配
- **核心函数**: `linx_rule_set_match_event()`

## 性能优化效果

### 匹配流程优化

#### 优化前
```
事件到达 -> 遍历所有规则 -> 逐一匹配 -> 返回结果
时间复杂度: O(n), n为规则总数
```

#### 优化后
```
事件到达 -> 事件分类 -> 查找对应规则子集 -> 匹配子集 -> 返回结果
时间复杂度: O(k), k为特定分类下的规则数 (k << n)
```

### 预期性能提升

假设有1000条规则，平均分布在10个不同的事件源中：
- **优化前**: 每次匹配需要尝试1000条规则
- **优化后**: 每次匹配只需要尝试约100条规则
- **性能提升**: 约10倍

## 使用方法

### 1. 初始化
```c
linx_rule_set_init();
linx_rule_analyzer_init();
```

### 2. 添加规则
```c
/* 系统会自动分析规则并分类 */
linx_rule_set_add(rule, match, output, NULL);
```

### 3. 事件匹配
```c
/* 使用新的事件匹配接口 */
bool matched = linx_rule_set_match_event(&event);
```

### 4. 获取性能统计
```c
struct stats_t stats;
linx_rule_set_get_stats(&stats);
printf("跳过规则数: %zu\n", stats.skipped_rules);
printf("过滤效率: %.2f%%\n", 
       (double)stats.skipped_rules / stats.match_attempts * 100.0);
```

## 兼容性

### 向后兼容
- 保留了原有的 `linx_rule_set_match_rule()` 接口
- 现有的规则加载逻辑无需修改
- 规则文件格式保持不变

### 新特性
- 新增 `linx_rule_set_match_event()` 接口，提供更高的性能
- 新增性能统计功能
- 支持规则自动分类

## 规则分类规则

### 自动分类
系统会自动分析规则条件中的以下模式：
- `evt.type = open` -> 文件I/O类规则
- `evt.type = socket` -> 网络类规则
- `evt.dir = <` -> 进入类规则
- `evt.dir = >` -> 退出类规则

### 通用规则
以下情况的规则会被归类为通用规则：
- 只包含 `proc.name`、`proc.cmdline` 等进程信息的规则
- 没有明确事件类型限制的规则
- 分析失败的规则

## 测试验证

使用 `linx_rule_set_performance_test` 程序可以测试优化效果：

```bash
cd /workspace/userspace/linx_rule_engine/rule_engine_set
gcc -o test linx_rule_set_performance_test.c -I../include -L../lib -llinx_rule_engine
./test
```

## 注意事项

1. **内存使用**: 三级索引结构会占用更多内存，但相对于性能提升是值得的
2. **规则分析**: 复杂的规则条件可能无法正确分类，会自动归为通用规则
3. **维护成本**: 新增事件类型时需要更新事件分类映射表

## 未来优化方向

1. **动态负载均衡**: 根据实际匹配频率动态调整规则分组
2. **缓存机制**: 为频繁匹配的事件类型添加结果缓存
3. **并行匹配**: 利用多线程并行匹配不同分类的规则
4. **机器学习**: 使用ML算法优化规则分类和匹配策略