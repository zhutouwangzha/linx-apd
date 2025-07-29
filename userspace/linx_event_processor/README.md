# LINX Event Processor - 事件处理器模块

## 📋 模块概述

`linx_event_processor` 是系统的事件处理器模块，负责协调事件的处理流程，包括事件分发、并行处理和结果聚合。

## 🎯 核心功能

- **事件分发**: 将事件分发到不同的处理器
- **并行处理**: 支持多线程并行事件处理
- **处理器管理**: 管理各种事件处理器的生命周期
- **性能监控**: 监控事件处理性能和统计信息

## 🔧 核心接口

```c
// 事件处理器初始化
int linx_event_processor_init(void);
void linx_event_processor_deinit(void);

// 事件处理
int linx_event_processor_process(linx_event_t *event);
int linx_event_processor_process_batch(linx_event_t **events, int count);

// 处理器配置
typedef struct {
    int worker_threads;             // 工作线程数
    int queue_size;                 // 队列大小
    int batch_size;                 // 批处理大小
    bool enable_parallel;           // 启用并行处理
} processor_config_t;

int linx_event_processor_configure(processor_config_t *config);
```

## 🏗️ 模块结构

```
linx_event_processor/
├── include/
│   ├── linx_event_processor.h          # 主要接口
│   ├── linx_event_processor_config.h   # 配置管理
│   ├── linx_event_processor_task.h     # 任务管理
│   └── linx_event_processor_define.h   # 定义和宏
├── linx_event_processor.c              # 核心实现
└── Makefile                            # 构建配置
```

## 🔗 模块依赖

- `linx_thread` - 线程管理
- `linx_event_queue` - 事件队列
- `linx_log` - 日志输出
