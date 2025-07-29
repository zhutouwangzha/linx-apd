# LINX Log - 日志管理模块

## 📋 模块概述

`linx_log` 是系统的统一日志管理模块，提供多级别、多输出的日志记录功能，支持结构化日志、日志轮转和实时日志监控。

## 🎯 核心功能

- **多级别日志**: 支持DEBUG、INFO、WARN、ERROR、FATAL等级别
- **多输出方式**: 支持文件、stderr、stdout等输出方式
- **日志轮转**: 自动日志文件轮转和压缩
- **结构化日志**: 支持JSON格式的结构化日志输出
- **性能优化**: 异步日志写入和缓冲机制

## 🔧 核心接口

```c
// 日志系统初始化
int linx_log_init(const char *output_path, const char *log_level);
void linx_log_deinit(void);

// 基本日志宏
#define LINX_LOG_DEBUG(fmt, ...)
#define LINX_LOG_INFO(fmt, ...)
#define LINX_LOG_WARN(fmt, ...)
#define LINX_LOG_ERROR(fmt, ...)
#define LINX_LOG_FATAL(fmt, ...)

// 日志级别控制
int linx_log_set_level(const char *level);
const char *linx_log_get_level(void);

// 日志配置
int linx_log_set_output(const char *output_path);
int linx_log_set_format(const char *format); // "text" or "json"
```

## 📊 日志级别

| 级别 | 数值 | 描述 | 用途 |
|------|------|------|------|
| DEBUG | 0 | 调试信息 | 开发调试使用 |
| INFO | 1 | 一般信息 | 正常运行日志 |
| WARN | 2 | 警告信息 | 潜在问题提醒 |
| ERROR | 3 | 错误信息 | 错误事件记录 |
| FATAL | 4 | 致命错误 | 严重错误，程序退出 |

## 🔧 配置选项

```yaml
log:
  output: "/var/log/linx_apd/linx_apd.log"  # stderr, stdout, 或文件路径
  level: "INFO"                             # 日志级别
  format: "text"                            # text 或 json
  rotation:
    enabled: true                           # 启用日志轮转
    max_size: "100MB"                       # 最大文件大小
    max_files: 10                           # 保留文件数
    compress: true                          # 压缩旧文件
```

## 📝 使用示例

```c
#include "linx_log.h"

int main() {
    // 初始化日志系统
    linx_log_init("/var/log/linx_apd/app.log", "INFO");
    
    // 记录不同级别的日志
    LINX_LOG_INFO("Application started");
    LINX_LOG_DEBUG("Debug information: value=%d", 42);
    LINX_LOG_WARN("This is a warning message");
    LINX_LOG_ERROR("Error occurred: %s", strerror(errno));
    
    // 清理资源
    linx_log_deinit();
    return 0;
}
```