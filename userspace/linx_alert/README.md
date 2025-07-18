# LINX 多线程告警输出系统

## 概述

LINX 多线程告警输出系统是一个高性能的告警处理模块，专为 LINX 安全监控系统设计。它提供了异步、多线程的告警输出能力，支持多种输出方式，包括 stdout、文件、HTTP 和 syslog。

## 主要特性

- **多线程异步处理** - 使用线程池避免 I/O 阻塞
- **多种输出后端** - 支持 stdout、file、HTTP、syslog
- **格式化输出** - 与 `linx_output_match_format` 无缝集成
- **配置灵活** - 运行时动态配置输出方式
- **线程安全** - 完全的线程安全设计
- **性能监控** - 内置统计功能

## 快速开始

### 1. 编译

```bash
cd userspace/linx_alert
make all
```

### 2. 基本使用

```c
#include "linx_alert.h"

int main() {
    // 初始化系统（4个工作线程）
    linx_alert_init(4);
    
    // 配置 stdout 输出
    linx_output_config_t config = {0};
    config.type = LINX_OUTPUT_TYPE_STDOUT;
    config.enabled = true;
    config.config.stdout_config.use_color = true;
    config.config.stdout_config.timestamp = true;
    linx_alert_add_output_config(&config);
    
    // 发送告警（假设已有 output_match）
    linx_alert_format_and_send(output_match, "test_rule", 2);
    
    // 清理资源
    linx_alert_cleanup();
    return 0;
}
```

### 3. 运行示例

```bash
make test
```

## API 参考

### 初始化和清理

- `int linx_alert_init(int thread_pool_size)` - 初始化告警系统
- `void linx_alert_cleanup(void)` - 清理系统资源

### 配置管理

- `int linx_alert_add_output_config(linx_output_config_t *config)` - 添加输出配置
- `int linx_alert_remove_output_config(linx_output_type_t type)` - 移除输出配置

### 告警发送

- `int linx_alert_format_and_send(linx_output_match_t *output_match, const char *rule_name, int priority)` - 格式化并发送告警（推荐）
- `int linx_alert_send_async(linx_output_match_t *output_match, const char *rule_name, int priority)` - 异步发送
- `int linx_alert_send_sync(linx_output_match_t *output_match, const char *rule_name, int priority)` - 同步发送

### 统计信息

- `void linx_alert_get_stats(long *total_sent, long *total_failed)` - 获取统计信息
- `void linx_alert_reset_stats(void)` - 重置统计信息

## 输出配置

### stdout 输出

```c
linx_output_config_t stdout_config = {0};
stdout_config.type = LINX_OUTPUT_TYPE_STDOUT;
stdout_config.enabled = true;
stdout_config.config.stdout_config.use_color = true;      // 彩色输出
stdout_config.config.stdout_config.timestamp = true;     // 显示时间戳
```

### 文件输出

```c
linx_output_config_t file_config = {0};
file_config.type = LINX_OUTPUT_TYPE_FILE;
file_config.enabled = true;
file_config.config.file_config.file_path = strdup("/var/log/linx.log");
file_config.config.file_config.max_file_size = 100 * 1024 * 1024;  // 100MB
file_config.config.file_config.max_files = 10;                     // 保留10个文件
file_config.config.file_config.append = true;                      // 追加模式
```

### HTTP 输出

```c
linx_output_config_t http_config = {0};
http_config.type = LINX_OUTPUT_TYPE_HTTP;
http_config.enabled = true;
http_config.config.http_config.url = strdup("http://localhost:8080/alerts");
http_config.config.http_config.timeout_ms = 5000;                  // 5秒超时
http_config.config.http_config.headers = strdup("Authorization: Bearer token");
```

### syslog 输出

```c
linx_output_config_t syslog_config = {0};
syslog_config.type = LINX_OUTPUT_TYPE_SYSLOG;
syslog_config.enabled = true;
syslog_config.config.syslog_config.facility = 16;     // LOG_LOCAL0
syslog_config.config.syslog_config.priority = -1;     // 使用消息优先级
```

## 优先级定义

- `0` - CRITICAL (紧急)
- `1` - ERROR (错误)
- `2` - WARNING (警告)
- `3` - INFO (信息)

## 与规则引擎集成

在规则匹配时调用告警系统：

```c
// 规则匹配回调函数
void on_rule_match(linx_rule_t *rule, linx_output_match_t *output_match) {
    const char *rule_name = rule->name;
    int priority = rule->priority;
    
    // 发送告警
    int result = linx_alert_format_and_send(output_match, rule_name, priority);
    if (result != 0) {
        // 处理错误
        printf("Failed to send alert for rule: %s\n", rule_name);
    }
}
```

## 性能调优

### 线程池大小

- **CPU 密集型任务** - 设置为 CPU 核心数
- **I/O 密集型任务** - 设置为 CPU 核心数的 2-4 倍
- **默认推荐** - 4 个线程

### 文件输出优化

- 设置合适的文件大小限制（10-100MB）
- 启用日志轮转避免单个文件过大
- 使用 SSD 存储提高写入性能

### HTTP 输出优化

- 设置合理的超时时间（1-10秒）
- 考虑批量发送减少网络开销
- 使用连接池复用连接

## 故障排除

### 常见问题

1. **编译错误** - 确保安装了 pthread 库和相关头文件
2. **运行时崩溃** - 检查是否正确初始化和清理系统
3. **输出丢失** - 检查输出配置是否正确，权限是否足够
4. **性能问题** - 调整线程池大小，检查 I/O 瓶颈

### 调试

启用调试模式：

```bash
make debug
```

使用 GDB 调试：

```bash
gdb ./example/linx_alert_example
```

### 日志输出

系统会将错误信息输出到 stderr，可以重定向到文件：

```bash
./your_program 2> error.log
```

## 扩展开发

### 添加新的输出后端

1. 在 `linx_output_type_t` 枚举中添加新类型
2. 在 `linx_output_config_t` 联合体中添加配置结构
3. 实现输出函数 `int linx_alert_output_xxx(...)`
4. 在 `linx_alert_send_to_outputs` 中添加调用

### 自定义格式化

修改 `linx_output_match_format` 函数以支持新的变量类型。

## 许可证

本项目采用 [您的许可证] 许可证。

## 贡献

欢迎提交 Issue 和 Pull Request。

## 更多文档

- [完整系统设计文档](LINX_ALERT_SYSTEM_DESIGN.md)
- [API 详细文档](docs/api.md)
- [性能测试报告](docs/performance.md)