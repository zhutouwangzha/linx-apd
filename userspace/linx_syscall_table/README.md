# LINX Syscall Table - 系统调用表模块

## 📋 模块概述

`linx_syscall_table` 是系统调用表管理模块，维护系统调用号到系统调用名称的映射关系，为事件处理提供系统调用信息查询服务。

## 🎯 核心功能

- **系统调用映射**: 系统调用号到名称的双向映射
- **架构支持**: 支持多种CPU架构的系统调用表
- **快速查询**: 高效的系统调用信息查询
- **动态加载**: 支持从配置文件动态加载系统调用表

## 🔧 核心接口

```c
// 系统调用表初始化
int linx_syscall_table_init(void);
void linx_syscall_table_deinit(void);

// 查询接口
const char *linx_syscall_get_name(int syscall_id);
int linx_syscall_get_id(const char *syscall_name);
bool linx_syscall_is_valid(int syscall_id);

// 系统调用信息
typedef struct {
    int id;                         // 系统调用号
    char name[64];                  // 系统调用名称
    int param_count;                // 参数数量
    bool is_interesting;            // 是否为感兴趣的系统调用
} syscall_info_t;

syscall_info_t *linx_syscall_get_info(int syscall_id);
```

## 📊 支持的架构

- x86_64 (AMD64)
- i386 (x86)
- ARM64 (AArch64)
- ARM (32-bit)

## 🔗 模块依赖

- `linx_log` - 日志输出