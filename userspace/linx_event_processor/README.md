# LINX Event Processor

LINX事件处理器模块，用于创建和管理两个线程池来处理事件获取和规则匹配。

## 功能特性

- **双线程池架构**：
  - 事件获取线程池：CPU核数个线程，用于从eBPF环形缓冲区获取原始事件
  - 规则匹配线程池：CPU核数×2个线程，用于事件规则匹配和告警输出

- **线程安全**：
  - 使用互斥锁保护共享数据结构
  - 统计信息的原子更新
  - 优雅的启动和停止机制

- **高性能**：
  - 异步事件处理
  - 无锁队列设计
  - 批处理支持

- **可监控**：
  - 详细的统计信息
  - 实时状态查询
  - 性能指标收集

## 架构设计

```
eBPF Ring Buffer
       ↓
Event Fetch Workers (CPU核数个线程)
       ↓
Event Enrichment
       ↓
Event Match Workers (CPU核数×2个线程)
       ↓
Rule Engine → Alert System
```

## API接口

### 初始化和清理

```c
// 初始化事件处理器
int linx_event_processor_init(linx_event_processor_config_t *config);

// 销毁事件处理器
void linx_event_processor_deinit(void);
```

### 运行控制

```c
// 启动事件处理器
int linx_event_processor_start(linx_ebpf_t *ebpf_manager);

// 停止事件处理器
int linx_event_processor_stop(void);
```

### 状态查询

```c
// 检查是否正在运行
bool linx_event_processor_is_running(void);

// 获取统计信息
void linx_event_processor_get_stats(linx_event_processor_stats_t *stats);
```

## 配置参数

```c
typedef struct {
    int event_fetcher_pool_size;    // 事件获取线程池大小
    int event_matcher_pool_size;    // 事件匹配线程池大小
    int event_queue_size;           // 事件队列大小
    int batch_size;                 // 批处理大小
} linx_event_processor_config_t;
```

## 统计信息

```c
typedef struct {
    uint64_t total_events;          // 总事件数
    uint64_t processed_events;      // 已处理事件数
    uint64_t matched_events;        // 匹配成功事件数
    uint64_t failed_events;         // 处理失败事件数
    uint64_t fetch_tasks_submitted; // 提交的获取任务数
    uint64_t match_tasks_submitted; // 提交的匹配任务数
    
    int fetcher_active_threads;     // 活跃的获取线程数
    int matcher_active_threads;     // 活跃的匹配线程数
    int fetcher_queue_size;         // 获取线程池队列大小
    int matcher_queue_size;         // 匹配线程池队列大小
    
    bool running;                   // 是否正在运行
} linx_event_processor_stats_t;
```

## 使用示例

```c
#include "linx_event_processor.h"

int main() {
    linx_event_processor_config_t config = {
        .event_fetcher_pool_size = 4,
        .event_matcher_pool_size = 8,
        .event_queue_size = 1000,
        .batch_size = 10
    };
    
    linx_ebpf_t ebpf_manager;
    
    // 初始化eBPF管理器
    linx_ebpf_init(&ebpf_manager);
    
    // 初始化事件处理器
    if (linx_event_processor_init(&config) != 0) {
        return -1;
    }
    
    // 启动事件处理器
    if (linx_event_processor_start(&ebpf_manager) != 0) {
        linx_event_processor_deinit();
        return -1;
    }
    
    // 处理事件...
    while (running) {
        sleep(1);
        
        // 获取统计信息
        linx_event_processor_stats_t stats;
        linx_event_processor_get_stats(&stats);
        printf("Processed: %lu events\n", stats.processed_events);
    }
    
    // 停止和清理
    linx_event_processor_stop();
    linx_event_processor_deinit();
    
    return 0;
}
```

## 依赖模块

- `linx_thread_pool`: 线程池实现
- `linx_ebpf_api`: eBPF接口
- `linx_rule_engine`: 规则引擎
- `linx_alert`: 告警系统
- `linx_log`: 日志系统
- `event.h`: 事件结构定义

## 线程安全保证

1. **数据结构保护**：使用互斥锁保护所有共享数据结构的访问
2. **原子操作**：统计计数器使用原子更新
3. **无锁队列**：基于线程池的任务队列是线程安全的
4. **优雅停止**：通过标志位控制线程的优雅退出

## 性能考虑

1. **CPU亲和性**：可以考虑将线程绑定到特定CPU核心
2. **内存池**：为频繁分配的事件结构使用内存池
3. **批处理**：支持批量处理多个事件以提高效率
4. **零拷贝**：尽量减少事件数据的拷贝操作

## 错误处理

- 所有函数返回错误码，0表示成功，-1表示失败
- 详细的日志记录帮助定位问题
- 资源泄漏保护，确保异常情况下正确清理资源

## 编译

```bash
cd /workspace/userspace/linx_event_processor
make
```

## 测试

运行示例程序：

```bash
./example_usage
```

## 注意事项

1. 确保在调用其他函数之前先调用`linx_event_processor_init()`
2. 程序退出前务必调用`linx_event_processor_deinit()`进行清理
3. 线程池大小应根据系统负载和CPU核数合理配置
4. 监控统计信息以优化性能参数