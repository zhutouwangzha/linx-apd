# LINX Hash Map - 哈希表工具模块

## 📋 模块概述

`linx_hash_map` 是系统的哈希表工具模块，提供高效的键值对存储和查询功能，主要用于字段绑定、缓存管理和快速数据查找。

## 🎯 核心功能

- **字段映射**: 实现规则字段到数据的快速绑定
- **类型管理**: 支持多种数据类型的存储和转换
- **高效查询**: 提供O(1)复杂度的数据查询
- **内存优化**: 高效的内存使用和哈希冲突处理

## 🔧 核心接口

```c
// 哈希表初始化
int linx_hash_map_init(void);
void linx_hash_map_deinit(void);

// 基本操作
int linx_hash_map_put(const char *key, void *value, linx_type_t type);
void *linx_hash_map_get(const char *key);
int linx_hash_map_remove(const char *key);
bool linx_hash_map_contains(const char *key);

// 字段绑定
int linx_field_bind(const char *field_name, void *data_ptr, size_t offset);
void *linx_field_get_value(const char *field_name);
linx_type_t linx_field_get_type(const char *field_name);
```

## 🏗️ 模块结构

```
linx_hash_map/
├── include/
│   ├── linx_hash_map.h         # 主要接口
│   ├── field_type.h            # 字段类型定义
│   ├── field_table.h           # 字段表管理
│   └── field_info.h            # 字段信息结构
├── linx_hash_map.c             # 哈希表实现
└── Makefile                    # 构建配置
```

## 📊 字段类型支持

```c
typedef enum {
    FIELD_TYPE_INT8,
    FIELD_TYPE_INT16,
    FIELD_TYPE_INT32,
    FIELD_TYPE_INT64,
    FIELD_TYPE_UINT8,
    FIELD_TYPE_UINT16,
    FIELD_TYPE_UINT32,
    FIELD_TYPE_UINT64,
    FIELD_TYPE_STRING,
    FIELD_TYPE_BOOLEAN,
} field_type_t;
```

## 🔗 模块依赖

- **uthash**: 哈希表实现库