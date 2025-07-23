#ifndef __LINX_EVENT_PROCESSOR_H__
#define __LINX_EVENT_PROCESSOR_H__ 

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/queue.h>

/* 前向声明 */
typedef struct linx_ebpf_s linx_ebpf_t;
typedef struct event_s event_t;

/* 事件处理状态 */
typedef enum {
    LINX_EVENT_PROC_STOPPED = 0,
    LINX_EVENT_PROC_STARTING,
    LINX_EVENT_PROC_RUNNING,
    LINX_EVENT_PROC_STOPPING,
    LINX_EVENT_PROC_ERROR
} linx_event_processor_state_t;

/* 事件处理优先级 */
typedef enum {
    LINX_EVENT_PRIORITY_LOW = 0,
    LINX_EVENT_PRIORITY_NORMAL,
    LINX_EVENT_PRIORITY_HIGH,
    LINX_EVENT_PRIORITY_CRITICAL
} linx_event_priority_t;

/* 事件队列配置 */
typedef struct {
    uint32_t capacity;              /* 队列容量 */
    uint32_t high_watermark;        /* 高水位标记 */
    uint32_t low_watermark;         /* 低水位标记 */
    bool drop_on_full;              /* 队列满时是否丢弃事件 */
    uint32_t batch_size;            /* 批处理大小 */
} linx_event_queue_config_t;

/* 线程池配置 */
typedef struct {
    uint32_t min_threads;           /* 最小线程数 */
    uint32_t max_threads;           /* 最大线程数 */
    uint32_t idle_timeout_ms;       /* 空闲超时时间(毫秒) */
    bool cpu_affinity;              /* 是否启用CPU亲和性 */
    int priority;                   /* 线程优先级 */
} linx_thread_pool_config_t;

/* 性能调优配置 */
typedef struct {
    bool enable_prefetch;           /* 启用预取优化 */
    bool enable_batch_processing;   /* 启用批处理 */
    bool enable_zero_copy;          /* 启用零拷贝 */
    uint32_t spin_lock_timeout_us;  /* 自旋锁超时时间(微秒) */
    uint32_t memory_pool_size;      /* 内存池大小 */
} linx_performance_config_t;

/* 监控配置 */
typedef struct {
    bool enable_metrics;            /* 启用指标收集 */
    uint32_t stats_interval_ms;     /* 统计间隔(毫秒) */
    bool enable_latency_tracking;   /* 启用延迟跟踪 */
    bool enable_cpu_profiling;      /* 启用CPU性能分析 */
} linx_monitoring_config_t;

/* 主配置结构 */
typedef struct {
    /* 核心配置 */
    uint32_t fetcher_thread_count;      /* 事件获取线程数 */
    uint32_t matcher_thread_count;      /* 事件匹配线程数 */
    
    /* 子系统配置 */
    linx_event_queue_config_t queue_config;
    linx_thread_pool_config_t fetcher_pool_config;
    linx_thread_pool_config_t matcher_pool_config;
    linx_performance_config_t perf_config;
    linx_monitoring_config_t monitor_config;
    
    /* 回调函数 */
    void (*error_callback)(const char *error_msg);
    void (*stats_callback)(const void *stats);
} linx_event_processor_config_t;

/* 事件包装结构 */
typedef struct linx_event_wrapper {
    event_t *event;                     /* 事件数据 */
    uint64_t timestamp_ns;              /* 纳秒级时间戳 */
    uint64_t sequence_id;               /* 序列号 */
    linx_event_priority_t priority;     /* 事件优先级 */
    uint32_t retry_count;               /* 重试次数 */
    void *context;                      /* 上下文数据 */
    
    /* 队列链表节点 */
    TAILQ_ENTRY(linx_event_wrapper) queue_entry;
} linx_event_wrapper_t;

/* 线程安全的事件队列 */
typedef struct {
    TAILQ_HEAD(event_queue_head, linx_event_wrapper) head;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    uint32_t count;
    uint32_t capacity;
    uint32_t high_watermark;
    uint32_t low_watermark;
    bool drop_on_full;
    
    /* 统计信息 */
    uint64_t enqueue_count;
    uint64_t dequeue_count;
    uint64_t drop_count;
    uint64_t peak_size;
} linx_event_queue_t;

/* 内存池管理 */
typedef struct {
    void **pool;                    /* 内存块池 */
    uint32_t size;                  /* 池大小 */
    uint32_t used;                  /* 已使用数量 */
    uint32_t block_size;            /* 块大小 */
    pthread_mutex_t mutex;          /* 访问锁 */
} linx_memory_pool_t;

/* 性能统计 */
typedef struct {
    /* 事件统计 */
    uint64_t total_events_received;
    uint64_t total_events_processed;
    uint64_t total_events_matched;
    uint64_t total_events_dropped;
    uint64_t total_events_failed;
    
    /* 吞吐量统计 */
    uint64_t events_per_second;
    uint64_t peak_events_per_second;
    double avg_processing_latency_us;
    double max_processing_latency_us;
    
    /* 线程池统计 */
    uint32_t fetcher_active_threads;
    uint32_t fetcher_idle_threads;
    uint32_t matcher_active_threads;
    uint32_t matcher_idle_threads;
    
    /* 队列统计 */
    uint32_t queue_current_size;
    uint32_t queue_peak_size;
    double queue_avg_wait_time_us;
    
    /* 资源使用统计 */
    uint64_t memory_usage_bytes;
    double cpu_usage_percent;
    uint64_t context_switches;
    
    /* 错误统计 */
    uint64_t allocation_failures;
    uint64_t thread_creation_failures;
    uint64_t queue_full_events;
    
    /* 时间戳 */
    uint64_t last_update_timestamp;
    uint64_t uptime_seconds;
} linx_event_processor_stats_t;

/* 工作线程结构 */
typedef struct {
    pthread_t thread_id;
    uint32_t worker_id;
    bool running;
    bool should_stop;
    pthread_mutex_t state_mutex;
    pthread_cond_t wake_cond;
    
    /* CPU亲和性 */
    int cpu_core;
    
    /* 统计信息 */
    uint64_t events_processed;
    uint64_t processing_time_ns;
    uint64_t last_activity_timestamp;
    
    void *processor_ctx;            /* 指向处理器上下文 */
} linx_worker_thread_t;

/* 高级线程池 */
typedef struct {
    linx_worker_thread_t *workers;
    uint32_t min_threads;
    uint32_t max_threads;
    uint32_t current_threads;
    uint32_t active_threads;
    uint32_t idle_threads;
    
    pthread_mutex_t pool_mutex;
    pthread_cond_t work_available;
    
    /* 动态扩缩容 */
    uint32_t load_threshold_high;
    uint32_t load_threshold_low;
    uint64_t last_resize_timestamp;
    
    /* 任务队列 */
    linx_event_queue_t *task_queue;
    
    void *(*worker_func)(void *arg);
} linx_advanced_thread_pool_t;

/* 主处理器结构 */
typedef struct linx_event_processor {
    /* 配置 */
    linx_event_processor_config_t config;
    
    /* 状态管理 */
    linx_event_processor_state_t state;
    pthread_mutex_t state_mutex;
    pthread_cond_t state_cond;
    
    /* 线程池 */
    linx_advanced_thread_pool_t *fetcher_pool;
    linx_advanced_thread_pool_t *matcher_pool;
    
    /* 事件队列 */
    linx_event_queue_t *input_queue;       /* 输入队列 */
    linx_event_queue_t *processing_queue;   /* 处理队列 */
    
    /* 内存管理 */
    linx_memory_pool_t *event_pool;
    linx_memory_pool_t *wrapper_pool;
    
    /* eBPF集成 */
    linx_ebpf_t *ebpf_manager;
    
    /* 监控和统计 */
    linx_event_processor_stats_t stats;
    pthread_mutex_t stats_mutex;
    pthread_t monitor_thread;
    bool monitor_running;
    
    /* 序列号生成 */
    uint64_t sequence_counter;
    pthread_mutex_t sequence_mutex;
    
    /* 错误处理 */
    char last_error[256];
    pthread_mutex_t error_mutex;
    
    /* 热配置更新 */
    bool config_dirty;
    pthread_mutex_t config_mutex;
    
    /* 性能分析 */
    struct {
        uint64_t start_time;
        uint64_t total_cycles;
        uint64_t fetch_cycles;
        uint64_t match_cycles;
    } profiling;
    
} linx_event_processor_t;

/* 核心API函数 */

/**
 * @brief 创建事件处理器实例
 * @param config 配置参数，NULL使用默认配置
 * @return 处理器实例指针，失败返回NULL
 */
linx_event_processor_t *linx_event_processor_create(const linx_event_processor_config_t *config);

/**
 * @brief 销毁事件处理器实例
 * @param processor 处理器实例
 */
void linx_event_processor_destroy(linx_event_processor_t *processor);

/**
 * @brief 启动事件处理器
 * @param processor 处理器实例
 * @param ebpf_manager eBPF管理器
 * @return 0成功，-1失败
 */
int linx_event_processor_start(linx_event_processor_t *processor, linx_ebpf_t *ebpf_manager);

/**
 * @brief 停止事件处理器
 * @param processor 处理器实例
 * @param timeout_ms 超时时间(毫秒)，0表示无限等待
 * @return 0成功，-1失败
 */
int linx_event_processor_stop(linx_event_processor_t *processor, uint32_t timeout_ms);

/**
 * @brief 获取处理器状态
 * @param processor 处理器实例
 * @return 当前状态
 */
linx_event_processor_state_t linx_event_processor_get_state(linx_event_processor_t *processor);

/**
 * @brief 获取统计信息
 * @param processor 处理器实例
 * @param stats 统计信息输出缓冲区
 * @return 0成功，-1失败
 */
int linx_event_processor_get_stats(linx_event_processor_t *processor, linx_event_processor_stats_t *stats);

/**
 * @brief 重置统计信息
 * @param processor 处理器实例
 * @return 0成功，-1失败
 */
int linx_event_processor_reset_stats(linx_event_processor_t *processor);

/* 配置管理API */

/**
 * @brief 获取默认配置
 * @param config 配置输出缓冲区
 */
void linx_event_processor_get_default_config(linx_event_processor_config_t *config);

/**
 * @brief 热更新配置
 * @param processor 处理器实例
 * @param config 新配置
 * @return 0成功，-1失败
 */
int linx_event_processor_update_config(linx_event_processor_t *processor, 
                                      const linx_event_processor_config_t *config);

/**
 * @brief 验证配置
 * @param config 配置参数
 * @return 0有效，-1无效
 */
int linx_event_processor_validate_config(const linx_event_processor_config_t *config);

/* 高级控制API */

/**
 * @brief 暂停事件处理
 * @param processor 处理器实例
 * @return 0成功，-1失败
 */
int linx_event_processor_pause(linx_event_processor_t *processor);

/**
 * @brief 恢复事件处理
 * @param processor 处理器实例
 * @return 0成功，-1失败
 */
int linx_event_processor_resume(linx_event_processor_t *processor);

/**
 * @brief 刷新所有队列
 * @param processor 处理器实例
 * @param timeout_ms 超时时间(毫秒)
 * @return 0成功，-1失败
 */
int linx_event_processor_flush(linx_event_processor_t *processor, uint32_t timeout_ms);

/**
 * @brief 手动触发垃圾回收
 * @param processor 处理器实例
 * @return 回收的内存字节数
 */
uint64_t linx_event_processor_gc(linx_event_processor_t *processor);

/* 调试和诊断API */

/**
 * @brief 导出内部状态
 * @param processor 处理器实例
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return 实际写入的字节数
 */
int linx_event_processor_dump_state(linx_event_processor_t *processor, 
                                   char *buffer, size_t buffer_size);

/**
 * @brief 获取最后的错误信息
 * @param processor 处理器实例
 * @return 错误信息字符串
 */
const char *linx_event_processor_get_last_error(linx_event_processor_t *processor);

/**
 * @brief 启用/禁用性能分析
 * @param processor 处理器实例
 * @param enable 是否启用
 * @return 0成功，-1失败
 */
int linx_event_processor_set_profiling(linx_event_processor_t *processor, bool enable);

/* 辅助宏定义 */
#define LINX_EVENT_PROCESSOR_DEFAULT_FETCHER_THREADS    4
#define LINX_EVENT_PROCESSOR_DEFAULT_MATCHER_THREADS    8
#define LINX_EVENT_PROCESSOR_DEFAULT_QUEUE_SIZE         10000
#define LINX_EVENT_PROCESSOR_DEFAULT_BATCH_SIZE         32
#define LINX_EVENT_PROCESSOR_DEFAULT_MEMORY_POOL_SIZE   1000

#define LINX_EVENT_PROCESSOR_MIN_THREADS                1
#define LINX_EVENT_PROCESSOR_MAX_THREADS                64
#define LINX_EVENT_PROCESSOR_MIN_QUEUE_SIZE             100
#define LINX_EVENT_PROCESSOR_MAX_QUEUE_SIZE             1000000

/* 错误码定义 */
#define LINX_EVENT_PROC_SUCCESS                 0
#define LINX_EVENT_PROC_ERROR_INVALID_PARAM     -1
#define LINX_EVENT_PROC_ERROR_OUT_OF_MEMORY     -2
#define LINX_EVENT_PROC_ERROR_THREAD_CREATE     -3
#define LINX_EVENT_PROC_ERROR_ALREADY_RUNNING   -4
#define LINX_EVENT_PROC_ERROR_NOT_RUNNING       -5
#define LINX_EVENT_PROC_ERROR_TIMEOUT           -6
#define LINX_EVENT_PROC_ERROR_QUEUE_FULL        -7
#define LINX_EVENT_PROC_ERROR_INTERNAL          -8

#endif /* __LINX_EVENT_PROCESSOR_H__ */
