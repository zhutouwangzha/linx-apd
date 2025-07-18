# LINX 多线程告警输出系统设计

## 概述

本文档描述了基于您的 falco C 语言实现的多线程告警输出系统。该系统设计用于在规则匹配时高效地格式化和输出告警消息，支持多种输出方式和异步处理。

## 系统架构

### 核心组件

1. **linx_alert 核心模块** - 主要的告警管理系统
2. **linx_output_match_format** - 格式化输出消息
3. **线程池** - 异步处理告警输出
4. **输出后端** - 支持多种输出方式

### 架构图

```
[规则引擎] 
    ↓ (规则匹配)
[linx_output_match_format] 
    ↓ (格式化消息)
[linx_alert_system]
    ↓ (异步/同步)
[线程池] → [输出后端们]
                ↓
    [stdout] [file] [http] [syslog]
```

## 数据结构设计

### 主要数据结构

```c
// 告警系统结构
typedef struct {
    linx_thread_pool_t *thread_pool;        // 线程池
    linx_output_config_t *output_configs;   // 输出配置数组
    int output_count;                        // 输出配置数量
    pthread_mutex_t config_mutex;           // 配置互斥锁
    bool initialized;                        // 初始化标志
    long total_alerts_sent;                 // 发送统计
    long total_alerts_failed;               // 失败统计
    pthread_mutex_t stats_mutex;            // 统计互斥锁
} linx_alert_system_t;

// 告警消息结构
typedef struct {
    char *formatted_message;                // 格式化后的消息
    size_t message_len;                      // 消息长度
    linx_output_config_t *output_configs;   // 输出配置
    int output_count;                        // 输出数量
    time_t timestamp;                        // 时间戳
    char *rule_name;                         // 规则名称
    int priority;                            // 优先级
} linx_alert_message_t;

// 输出配置结构
typedef struct {
    linx_output_type_t type;                 // 输出类型
    bool enabled;                            // 是否启用
    union {
        struct {
            bool use_color;                  // 使用颜色
            bool timestamp;                  // 显示时间戳
        } stdout_config;
        
        struct {
            char *file_path;                 // 文件路径
            size_t max_file_size;            // 最大文件大小
            int max_files;                   // 最大文件数
            bool append;                     // 追加模式
        } file_config;
        
        struct {
            char *url;                       // HTTP URL
            char *headers;                   // HTTP 头部
            int timeout_ms;                  // 超时时间
        } http_config;
        
        struct {
            int facility;                    // syslog facility
            int priority;                    // syslog priority
        } syslog_config;
    } config;
} linx_output_config_t;
```

## 核心功能

### 1. 系统初始化

```c
// 初始化告警系统，指定线程池大小
int linx_alert_init(int thread_pool_size);

// 清理告警系统资源
void linx_alert_cleanup(void);
```

### 2. 配置管理

```c
// 添加输出配置
int linx_alert_add_output_config(linx_output_config_t *config);

// 移除输出配置
int linx_alert_remove_output_config(linx_output_type_t type);

// 更新输出配置
int linx_alert_update_output_config(linx_output_type_t type, linx_output_config_t *config);
```

### 3. 告警发送

```c
// 异步发送告警（推荐）
int linx_alert_send_async(linx_output_match_t *output_match, const char *rule_name, int priority);

// 同步发送告警
int linx_alert_send_sync(linx_output_match_t *output_match, const char *rule_name, int priority);

// 格式化并发送告警（主要接口）
int linx_alert_format_and_send(linx_output_match_t *output_match, const char *rule_name, int priority);
```

### 4. 统计信息

```c
// 获取统计信息
void linx_alert_get_stats(long *total_sent, long *total_failed);

// 重置统计信息
void linx_alert_reset_stats(void);
```

## 与 linx_output_match_format 的交互

### 交互流程

1. **规则匹配触发** - 当规则引擎检测到匹配时
2. **调用格式化接口** - 调用 `linx_alert_format_and_send`
3. **消息格式化** - 内部调用 `linx_output_match_format` 处理变量替换
4. **任务提交** - 将格式化后的消息提交给线程池
5. **异步输出** - 线程池工作线程执行输出操作

### 代码示例

```c
// 在规则匹配时的调用示例
void on_rule_match(linx_rule_t *rule, linx_output_match_t *output_match) {
    // 获取规则名称和优先级
    const char *rule_name = rule->name;
    int priority = rule->priority;
    
    // 格式化并异步发送告警
    int result = linx_alert_format_and_send(output_match, rule_name, priority);
    if (result != 0) {
        // 处理发送失败的情况
        printf("Failed to send alert for rule: %s\n", rule_name);
    }
}
```

## 输出后端实现

### 1. stdout 输出
- 支持彩色输出
- 支持时间戳显示
- 根据优先级着色

### 2. 文件输出
- 支持日志轮转
- 可配置最大文件大小和数量
- 支持追加/覆盖模式

### 3. HTTP 输出
- JSON 格式化
- 可配置超时和头部
- 使用 curl 命令发送

### 4. syslog 输出
- 标准 syslog 协议
- 可配置 facility 和 priority
- 自动优先级映射

## 性能优化

### 多线程设计
- **线程池** - 避免频繁创建/销毁线程
- **异步处理** - 主线程不阻塞在 I/O 操作上
- **任务队列** - 平滑处理突发告警

### 内存管理
- **消息拷贝** - 避免共享数据竞争
- **及时释放** - 处理完成后立即清理内存
- **配置缓存** - 输出配置缓存在消息中

### 线程安全
- **配置互斥锁** - 保护输出配置的并发访问
- **统计互斥锁** - 保护统计信息的并发更新
- **无锁消息** - 每个告警消息独立处理

## 使用指南

### 1. 基本使用

```c
#include "linx_alert.h"

int main() {
    // 初始化系统
    linx_alert_init(4);  // 4个工作线程
    
    // 配置 stdout 输出
    linx_output_config_t config = {0};
    config.type = LINX_OUTPUT_TYPE_STDOUT;
    config.enabled = true;
    config.config.stdout_config.use_color = true;
    config.config.stdout_config.timestamp = true;
    linx_alert_add_output_config(&config);
    
    // 发送告警
    linx_alert_format_and_send(output_match, "test_rule", 2);
    
    // 清理
    linx_alert_cleanup();
    return 0;
}
```

### 2. 多输出配置

```c
// 同时配置多种输出方式
void setup_multiple_outputs() {
    // stdout
    linx_output_config_t stdout_config = {0};
    stdout_config.type = LINX_OUTPUT_TYPE_STDOUT;
    stdout_config.enabled = true;
    stdout_config.config.stdout_config.use_color = true;
    stdout_config.config.stdout_config.timestamp = true;
    linx_alert_add_output_config(&stdout_config);
    
    // file
    linx_output_config_t file_config = {0};
    file_config.type = LINX_OUTPUT_TYPE_FILE;
    file_config.enabled = true;
    file_config.config.file_config.file_path = strdup("/var/log/linx.log");
    file_config.config.file_config.max_file_size = 100 * 1024 * 1024;  // 100MB
    file_config.config.file_config.max_files = 10;
    file_config.config.file_config.append = true;
    linx_alert_add_output_config(&file_config);
    
    // syslog
    linx_output_config_t syslog_config = {0};
    syslog_config.type = LINX_OUTPUT_TYPE_SYSLOG;
    syslog_config.enabled = true;
    syslog_config.config.syslog_config.facility = 16;  // LOG_LOCAL0
    syslog_config.config.syslog_config.priority = -1;  // 使用消息优先级
    linx_alert_add_output_config(&syslog_config);
}
```

### 3. 与规则引擎集成

```c
// 在规则引擎中的集成示例
int process_rule_match(linx_rule_match_result_t *match_result) {
    if (!match_result || !match_result->output_match) {
        return -1;
    }
    
    // 获取规则信息
    const char *rule_name = match_result->rule_name;
    int priority = match_result->priority;
    
    // 发送告警
    return linx_alert_format_and_send(
        match_result->output_match, 
        rule_name, 
        priority
    );
}
```

## 配置示例

### 优先级定义
- 0: CRITICAL (紧急)
- 1: ERROR (错误)  
- 2: WARNING (警告)
- 3: INFO (信息)

### 输出格式示例
```
[2024-01-15 14:30:25] [WARNING] [suspicious_process] Alert: Process bash (PID: 1234) triggered rule at 2024-01-15T14:30:25Z - Suspicious command execution detected
```

## 扩展性

### 添加新的输出后端
1. 在 `linx_output_type_t` 枚举中添加新类型
2. 在 `linx_output_config_t` 联合体中添加配置结构
3. 实现输出函数 `int linx_alert_output_xxx(linx_alert_message_t *message, linx_output_config_t *config)`
4. 在 `linx_alert_send_to_outputs` 中添加相应的 case

### 自定义格式化
可以通过修改 `linx_output_match_format` 函数来支持更多的变量类型和格式化选项。

## 注意事项

1. **线程安全** - 所有公共接口都是线程安全的
2. **资源管理** - 调用 `linx_alert_cleanup()` 确保资源正确释放
3. **错误处理** - 检查所有函数的返回值
4. **性能监控** - 定期检查统计信息以监控系统性能
5. **配置验证** - 添加输出配置前进行有效性检查

## 总结

这个多线程告警输出系统提供了：
- **高性能** - 异步处理，避免 I/O 阻塞
- **灵活性** - 支持多种输出方式和配置
- **可扩展** - 易于添加新的输出后端
- **可靠性** - 完整的错误处理和资源管理
- **易用性** - 简单的 API 设计

通过与 `linx_output_match_format` 的无缝集成，系统能够高效地处理规则匹配后的告警输出，为安全监控系统提供强大的告警能力。