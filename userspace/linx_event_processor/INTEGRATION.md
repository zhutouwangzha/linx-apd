# LINX Event Processor 集成说明

本文档说明如何将 LINX Event Processor 与现有的 `linx_thread_pool` 模块集成使用。

## 集成架构

LINX Event Processor 现在完全基于您现有的 `linx_thread_pool` 模块构建，保持了与现有代码库的一致性。

### 线程池使用方式

```c
// 创建获取线程池
processor->fetcher_pool = linx_thread_pool_create(fetcher_thread_count);

// 创建匹配线程池  
processor->matcher_pool = linx_thread_pool_create(matcher_thread_count);

// 提交任务到线程池
linx_thread_pool_add_task(processor->fetcher_pool, event_fetch_worker, task_arg);
linx_thread_pool_add_task(processor->matcher_pool, event_match_worker, task_arg);
```

### 任务函数签名

所有工作函数都遵循 `linx_thread_pool` 的函数签名：
```c
void *worker_function(void *arg, int *should_stop);
```

其中：
- `arg`: 任务参数，包含处理器上下文和事件数据
- `should_stop`: 线程池的停止标志

### 任务参数结构

```c
typedef struct {
    linx_task_type_t type;                  // 任务类型
    linx_event_processor_t *processor;      // 处理器实例
    linx_event_wrapper_t *event_wrapper;    // 事件包装器（仅匹配任务）
    int worker_id;                          // 工作线程ID
} linx_task_arg_t;
```

## 线程管理

### 事件获取线程

- **数量**: 默认等于CPU核心数
- **职责**: 从eBPF环形缓冲区持续获取事件
- **工作模式**: 长期运行任务，直到停止信号

```c
static void *event_fetch_worker(void *arg, int *should_stop)
{
    linx_task_arg_t *task_arg = (linx_task_arg_t *)arg;
    linx_event_processor_t *processor = task_arg->processor;
    
    while (!*should_stop && !processor->shutdown_requested) {
        // 获取事件并提交到匹配线程池
        // ...
    }
    
    return NULL;
}
```

### 事件匹配线程

- **数量**: 默认等于CPU核心数×2
- **职责**: 执行规则匹配和告警处理
- **工作模式**: 短期任务，处理单个事件后结束

```c
static void *event_match_worker(void *arg, int *should_stop)
{
    linx_task_arg_t *task_arg = (linx_task_arg_t *)arg;
    // 处理单个事件
    // 释放资源并退出
    free(task_arg);
    return NULL;
}
```

## 统计信息集成

通过现有线程池的API获取统计信息：

```c
// 获取活跃线程数
stats->fetcher_active_threads = linx_thread_pool_get_active_threads(processor->fetcher_pool);
stats->matcher_active_threads = linx_thread_pool_get_active_threads(processor->matcher_pool);

// 获取队列大小
stats->fetcher_queue_size = linx_thread_pool_get_queue_size(processor->fetcher_pool);
stats->matcher_queue_size = linx_thread_pool_get_queue_size(processor->matcher_pool);
```

## 优雅停止

停止过程使用现有线程池的优雅停止机制：

```c
int linx_event_processor_stop(linx_event_processor_t *processor, uint32_t timeout_ms)
{
    // 设置停止标志
    processor->shutdown_requested = true;
    
    // 优雅停止线程池
    linx_thread_pool_destroy(processor->fetcher_pool, 1);  // graceful=1
    linx_thread_pool_destroy(processor->matcher_pool, 1);
    
    // 重新创建线程池以备下次使用
    processor->fetcher_pool = linx_thread_pool_create(fetcher_thread_count);
    processor->matcher_pool = linx_thread_pool_create(matcher_thread_count);
}
```

## 错误处理

所有错误都通过现有的错误处理机制报告：

```c
if (linx_thread_pool_add_task(pool, worker, arg) != 0) {
    set_last_error(processor, "Failed to submit task to thread pool");
    return LINX_EVENT_PROC_ERROR_THREAD_CREATE;
}
```

## 配置映射

Event Processor配置映射到线程池参数：

| Event Processor 配置 | 线程池参数 | 说明 |
|---------------------|-----------|------|
| `fetcher_thread_count` | `num_threads` | 获取线程池大小 |
| `matcher_thread_count` | `num_threads` | 匹配线程池大小 |
| `cpu_affinity` | 任务内部实现 | CPU亲和性设置 |
| `priority` | 任务内部实现 | 线程优先级设置 |

## 兼容性说明

### 与现有代码的兼容性

1. **完全兼容**: 使用标准的 `linx_thread_pool` API
2. **无需修改**: 现有线程池代码无需任何改动
3. **统一管理**: 所有线程都通过相同的池进行管理

### 性能考虑

1. **任务开销**: 匹配任务为短期任务，有一定的创建/销毁开销
2. **内存管理**: 使用内存池减少频繁分配
3. **负载均衡**: 依赖现有线程池的负载均衡机制

### 限制和注意事项

1. **动态扩缩容**: 当前实现不支持运行时动态调整线程数
2. **线程亲和性**: 需要在任务函数内部实现CPU绑定
3. **优先级调度**: 依赖现有线程池的调度策略

## 使用示例

### 基本用法

```c
// 创建处理器（使用现有线程池）
linx_event_processor_t *processor = linx_event_processor_create(NULL);

// 启动（内部创建并使用线程池）
linx_event_processor_start(processor, &ebpf_manager);

// 运行...

// 停止（优雅停止线程池）
linx_event_processor_stop(processor, 5000);

// 销毁
linx_event_processor_destroy(processor);
```

### 自定义配置

```c
linx_event_processor_config_t config;
linx_event_processor_get_default_config(&config);

// 调整线程数
config.fetcher_thread_count = 4;
config.matcher_thread_count = 8;

// 启用CPU亲和性
config.fetcher_pool_config.cpu_affinity = true;

linx_event_processor_t *processor = linx_event_processor_create(&config);
```

## 总结

通过集成现有的 `linx_thread_pool` 模块，LINX Event Processor 实现了：

- ✅ 与现有代码库的完美兼容
- ✅ 统一的线程管理机制  
- ✅ 一致的错误处理和统计
- ✅ 熟悉的API和使用方式
- ✅ 保持了高性能和可扩展性

这种设计确保了新的事件处理器能够无缝集成到现有的LINX系统中，同时提供了先进的事件处理能力。