# 多线程事件丰富化解决方案

## 问题描述

原始代码中存在两个主要的线程安全问题：

1. **`linx_event_rich.c` 中的 `static event_t evt`**：全局静态变量，多线程访问时会产生竞争条件
2. **`linx_hash_map.c` 中的 `static linx_hash_map_t *s_linx_hash_map`**：全局哈希表实例，多线程访问时需要更新上下文地址

## 解决方案

### 1. 线程上下文设计

创建了 `linx_thread_context_t` 结构，为每个线程提供独立的：
- `event_t evt`：事件结构实例
- `linx_hash_map_t *hash_map`：哈希表实例
- 其他线程特定数据

### 2. 核心文件修改

#### `linx_thread_context.h/c`
- 线程上下文管理
- 使用 `pthread_key_t` 实现线程特定数据存储
- 提供创建、销毁、获取上下文的API

#### `linx_event_rich.c` 修改
- 移除全局静态变量 `static event_t evt`
- 所有函数改为使用 `linx_thread_context_get_event()` 获取当前线程的事件结构
- 添加线程上下文初始化到 `linx_event_rich_init()`

#### `linx_hash_map.c` 修改
- 移除全局静态变量 `static linx_hash_map_t *s_linx_hash_map`
- 所有函数改为使用 `linx_thread_context_get_hashmap()` 获取当前线程的哈希表
- 保留原有API兼容性

#### `linx_event_rich_mt.h/c`
- 专门的多线程API接口
- 线程池工作函数示例
- 线程安全的事件处理

## 使用方法

### 单线程使用（兼容原有代码）

```c
#include "linx_event_rich.h"

int main() {
    // 原有的初始化方式仍然有效
    linx_event_rich_init();
    
    // 处理事件
    linx_event_t *event = get_event_from_somewhere();
    linx_event_rich(event);
    
    // 获取结果
    event_t *rich_event = linx_event_rich_get();
    
    // 清理
    linx_event_rich_deinit();
    return 0;
}
```

### 多线程使用

```c
#include "linx_event_rich_mt.h"

// 主线程初始化
int main() {
    // 初始化多线程系统
    linx_event_rich_mt_init();
    
    // 创建工作线程...
    pthread_t worker_threads[4];
    for (int i = 0; i < 4; i++) {
        pthread_create(&worker_threads[i], NULL, worker_function, NULL);
    }
    
    // 等待线程结束...
    for (int i = 0; i < 4; i++) {
        pthread_join(worker_threads[i], NULL);
    }
    
    // 清理
    linx_event_rich_mt_deinit();
    return 0;
}

// 工作线程函数
void *worker_function(void *arg) {
    // 初始化当前线程上下文
    linx_event_rich_thread_init();
    
    while (!stop_flag) {
        linx_event_t *event = get_event_from_queue();
        if (event) {
            // 线程安全的事件处理
            linx_event_rich_mt(event);
            
            // 获取结果
            event_t *rich_event = linx_event_rich_mt_get();
            
            // 处理结果...
        }
    }
    
    // 清理当前线程上下文
    linx_event_rich_thread_deinit();
    return NULL;
}
```

### 使用预定义的工作线程函数

```c
#include "linx_event_rich_mt.h"

int main() {
    linx_event_rich_mt_init();
    
    // 设置工作参数
    linx_worker_args_t worker_args = {
        .thread_id = 0,
        .event_queue = your_event_queue,
        .user_data = your_data,
        .stop_flag = &stop_flag
    };
    
    pthread_t worker_thread;
    pthread_create(&worker_thread, NULL, linx_event_rich_worker_thread, &worker_args);
    
    // ... 生产事件到队列
    
    pthread_join(worker_thread, NULL);
    linx_event_rich_mt_deinit();
    return 0;
}
```

## API 参考

### 多线程初始化/清理

```c
// 初始化多线程系统（主线程调用）
int linx_event_rich_mt_init(void);

// 清理多线程系统（主线程调用）
void linx_event_rich_mt_deinit(void);
```

### 线程上下文管理

```c
// 初始化当前线程上下文（每个工作线程调用）
int linx_event_rich_thread_init(void);

// 清理当前线程上下文（每个工作线程调用）
void linx_event_rich_thread_deinit(void);
```

### 事件处理

```c
// 多线程安全的事件处理
int linx_event_rich_mt(linx_event_t *event);

// 获取当前线程的事件结果
event_t *linx_event_rich_mt_get(void);
```

### 线程上下文直接访问

```c
// 获取当前线程上下文
linx_thread_context_t *linx_thread_context_get(void);

// 获取当前线程的事件结构
event_t *linx_thread_context_get_event(void);

// 获取当前线程的哈希表
linx_hash_map_t *linx_thread_context_get_hashmap(void);
```

## 编译和构建

```bash
# 构建库和示例
make all

# 仅构建库
make libs

# 仅构建示例
make example

# 清理
make clean

# 运行示例
./multithread_example
```

## 性能考虑

1. **内存使用**：每个线程都有独立的 `event_t` 和 `linx_hash_map_t` 实例，会增加内存使用
2. **初始化开销**：每个线程都需要初始化自己的字段映射表
3. **无锁设计**：线程间无需同步，避免了锁竞争
4. **缓存友好**：每个线程访问自己的数据，减少缓存行冲突

## 注意事项

1. **线程生命周期**：确保在线程结束前调用 `linx_event_rich_thread_deinit()`
2. **错误处理**：检查所有初始化函数的返回值
3. **内存管理**：线程上下文会自动清理动态分配的内存
4. **兼容性**：原有的单线程代码无需修改即可使用

## 示例程序

`examples/multithread_example.c` 提供了完整的多线程使用示例，包括：
- 事件队列实现
- 多个工作线程
- 事件生产者和消费者模式
- 优雅的停止机制

## 故障排除

1. **段错误**：检查是否在使用前调用了 `linx_event_rich_thread_init()`
2. **内存泄漏**：确保每个 `thread_init()` 都有对应的 `thread_deinit()`
3. **性能问题**：考虑调整线程数量和事件队列大小

## 扩展性

这个设计支持：
- 动态调整工作线程数量
- 自定义事件队列实现
- 线程池模式
- 事件批处理优化