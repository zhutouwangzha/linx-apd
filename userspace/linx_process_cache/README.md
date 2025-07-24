# linx_process_cache 模块

该模块本质是期望减少频繁访问/proc文件系统的开销，支持快速获取进程信息、自动更新过期的进程信息，处理进程父子关系。

## 新增功能：事件驱动的短暂进程缓存

### 问题描述
在原始实现中，模块依赖定期扫描 `/proc` 目录来发现和更新进程信息。对于执行时间很短（几毫秒到几十毫秒）的进程，这种轮询机制很可能错过它们，导致缓存无法覆盖这些短暂进程。

### 解决方案
基于现有的 eBPF 基础设施，实现了事件驱动的进程缓存机制：

#### 1. eBPF 事件监听
- **fork/vfork 事件**：在 `fork.bpf.c` 中监听进程创建事件，立即缓存新进程
- **execve/execveat 事件**：在 `execve.bpf.c` 中监听进程执行事件，更新进程信息
- **exit/exit_group 事件**：在 `exit.bpf.c` 中监听进程退出事件，标记进程状态

#### 2. 事件驱动接口
```c
/* 新增的事件驱动 API */
int linx_process_cache_on_fork(pid_t new_pid, pid_t parent_pid);
int linx_process_cache_on_exec(pid_t pid, const char *filename, const char *argv, const char *envp);
int linx_process_cache_on_exit(pid_t pid, int exit_code);
int linx_process_cache_preload(pid_t pid);
```

#### 3. 智能缓存策略
- **立即缓存**：在 fork 事件时立即为新进程创建缓存条目
- **差异化保留**：短暂进程使用较短的缓存保留时间
- **混合模式**：事件驱动为主，定期扫描为辅

### 配置选项

```c
/* 启用事件驱动模式 */
#define LINX_PROCESS_CACHE_EVENT_DRIVEN_ENABLED 1

/* 事件驱动模式下的轮询间隔（秒） */
#define LINX_PROCESS_CACHE_EVENT_DRIVEN_POLL_INTERVAL 10

/* 短暂进程的缓存保留时间（秒） */
#define LINX_PROCESS_CACHE_SHORT_LIVED_RETAIN_TIME 5
```

### 性能优化

1. **快速创建**：优化 `create_process_info` 函数，关键信息读取失败时使用默认值而不是失败
2. **异步处理**：所有事件处理都通过线程池异步执行
3. **智能轮询**：在事件驱动模式下减少 `/proc` 扫描频率
4. **内存优化**：短暂进程使用更短的缓存保留时间

### 测试

运行测试程序验证功能：

```bash
make test
./build/bin/test_linx_process_cache
```

测试包括：
- 单个短暂进程缓存测试
- 快速 fork 多个短暂进程测试
- 进程退出后的缓存保留测试

### 架构示意

```
eBPF Programs          User Space
┌─────────────┐        ┌──────────────────┐
│ fork.bpf.c  │─────────▶│ on_fork()        │
│ execve.bpf.c│─────────▶│ on_exec()        │
│ exit.bpf.c  │─────────▶│ on_exit()        │
└─────────────┘        │                  │
                       │ Process Cache    │
┌─────────────┐        │ ┌──────────────┐ │
│ /proc scan  │─────────▶│ │ Hash Table   │ │
│ (backup)    │        │ │ + Metadata   │ │
└─────────────┘        │ └──────────────┘ │
                       └──────────────────┘
```

### 优势

1. **实时性**：事件驱动保证进程创建时立即缓存
2. **完整性**：捕获所有进程，包括极短暂的进程
3. **效率**：减少不必要的 `/proc` 扫描
4. **可靠性**：混合模式确保不遗漏任何进程