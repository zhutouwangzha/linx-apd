# LINX YAML - YAML解析模块

## 📋 模块概述

`linx_yaml` 是系统的YAML文件解析模块，提供统一的YAML解析接口，支持配置文件和规则文件的解析，为其他模块提供结构化的配置数据访问。

## 🎯 核心功能

- **YAML解析**: 解析YAML格式的配置和规则文件
- **节点操作**: 提供YAML节点的增删改查操作
- **类型转换**: 自动进行数据类型转换
- **错误处理**: 详细的解析错误信息和位置定位
- **内存管理**: 高效的内存使用和资源清理

## 🔧 核心接口

```c
// YAML文档加载
int linx_yaml_load_file(const char *filename, yaml_document_t *document);
int linx_yaml_load_string(const char *yaml_string, yaml_document_t *document);

// 节点访问
yaml_node_t *linx_yaml_get_node(yaml_document_t *document, const char *path);
char *linx_yaml_get_string(yaml_document_t *document, const char *path);
int linx_yaml_get_int(yaml_document_t *document, const char *path);
bool linx_yaml_get_bool(yaml_document_t *document, const char *path);

// 节点栈操作
int linx_yaml_stack_push(yaml_node_t *node);
yaml_node_t *linx_yaml_stack_pop(void);
yaml_node_t *linx_yaml_stack_top(void);

// 资源清理
void linx_yaml_cleanup(yaml_document_t *document);
```

## 🏗️ 模块结构

```
linx_yaml/
├── include/
│   ├── linx_yaml_get.h         # 节点访问接口
│   ├── linx_yaml_load.h        # 文档加载接口
│   ├── linx_yaml_node.h        # 节点操作接口
│   └── linx_yaml_stack.h       # 栈操作接口
├── linx_yaml_get.c             # 节点值获取实现
├── linx_yaml_load.c            # YAML文档加载
├── linx_yaml_node.c            # 节点操作实现
└── linx_yaml_stack.c           # 栈操作实现
```

## 📊 使用示例

```c
#include "linx_yaml_load.h"
#include "linx_yaml_get.h"

// 加载YAML文件
yaml_document_t document;
if (linx_yaml_load_file("/etc/linx_apd/config.yaml", &document) != 0) {
    fprintf(stderr, "Failed to load YAML file\n");
    return -1;
}

// 获取配置值
char *log_level = linx_yaml_get_string(&document, "log.level");
int thread_count = linx_yaml_get_int(&document, "alert.thread_pool_size");
bool enable_color = linx_yaml_get_bool(&document, "alert.stdout.use_color");

printf("Log level: %s\n", log_level);
printf("Thread count: %d\n", thread_count);
printf("Enable color: %s\n", enable_color ? "true" : "false");

// 清理资源
free(log_level);
linx_yaml_cleanup(&document);
```

## 🔗 模块依赖

- **libyaml**: YAML解析库
- `linx_log` - 日志输出