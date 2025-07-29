# LINX Arg Parser - 命令行参数解析模块

## 📋 模块概述

`linx_arg_parser` 是系统的命令行参数解析模块，负责解析和验证用户提供的命令行参数，为主程序提供配置文件路径、运行模式等关键参数。

## 🎯 核心功能

- **参数解析**: 解析命令行参数并进行验证
- **帮助信息**: 提供详细的使用帮助和版本信息
- **配置路径**: 管理配置文件和规则文件路径
- **运行模式**: 支持不同的运行模式（守护进程、前台等）

## 🔧 核心接口

```c
// 参数解析初始化
int linx_arg_init(void);

// 获取argp结构
const struct argp *linx_argp_get_argp(void);

// 获取解析后的配置
linx_arg_config_t *linx_arg_get_config(void);

// 参数配置结构
typedef struct {
    char *linx_apd_config;          // 主配置文件路径
    char *linx_apd_rules;           // 规则文件路径
    bool daemon_mode;               // 守护进程模式
    bool verbose;                   // 详细输出
    char *log_file;                 // 日志文件路径
    int log_level;                  // 日志级别
} linx_arg_config_t;
```

## 📝 支持的参数

```bash
Usage: linx-apd [OPTIONS]

Options:
  -c, --config=FILE         指定主配置文件路径
  -r, --rules=FILE          指定规则文件路径
  -d, --daemon             以守护进程模式运行
  -v, --verbose            启用详细输出
  -l, --log-file=FILE      指定日志文件路径
  -L, --log-level=LEVEL    设置日志级别 (DEBUG|INFO|WARN|ERROR)
  -h, --help               显示帮助信息
  -V, --version            显示版本信息

Examples:
  linx-apd -c /etc/linx_apd/config.yaml -r /etc/linx_apd/rules.yaml
  linx-apd --daemon --log-file /var/log/linx_apd.log
```

## 🔗 模块依赖

- **argp**: GNU argp参数解析库
- `linx_log` - 日志输出