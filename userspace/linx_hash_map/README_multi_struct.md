# linx_hash_map 多结构体基地址管理

## 概述

`linx_hash_map` 支持同时管理多个不同类型结构体的字段偏移信息和基地址。每个结构体类型通过唯一的表名进行区分。

## 核心概念

### 表（Table）
- 每个表代表一种结构体类型
- 通过 `table_name` 进行唯一标识
- 每个表有自己的基地址 (`base_addr`)
- 每个表包含该结构体类型的所有字段信息

### 字段（Field）
- 存储在特定表中的结构体字段信息
- 包含偏移量、大小、类型等元数据

## 主要功能

### 1. 基地址管理

```c
/* 更新指定表的基地址 */
int linx_hash_map_update_base_addr(const char *table_name, void *new_base_addr);

/* 获取指定表的基地址 */
void *linx_hash_map_get_base_addr(const char *table_name);
```

### 2. 表管理

```c
/* 列出所有表名 */
int linx_hash_map_list_tables(char ***table_names, size_t *count);

/* 释放表名列表 */
void linx_hash_map_free_table_list(char **table_names, size_t count);
```

### 3. 智能值访问

```c
/* 自动使用表的基地址获取字段值指针 */
static inline void *linx_hash_map_get_table_value_ptr(const char *table_name, const field_result_t *field);

/* 使用指定基地址获取字段值指针 */
static inline void *linx_hash_map_get_value_ptr(void *base_addr, const field_result_t *field);
```

## 使用场景

### 场景1：多种结构体类型
```c
typedef struct {
    int id;
    char name[32];
    float score;
} student_t;

typedef struct {
    int emp_id;
    char department[16];
    double salary;
} employee_t;

// 创建不同的表
linx_hash_map_create_table("student", NULL);
linx_hash_map_create_table("employee", NULL);

// 为不同表添加字段
linx_hash_map_add_field("student", "id", offsetof(student_t, id), sizeof(int), FIELD_TYPE_INT);
linx_hash_map_add_field("employee", "emp_id", offsetof(employee_t, emp_id), sizeof(int), FIELD_TYPE_INT);
```

### 场景2：同类型结构体的多个实例
```c
student_t student1 = {1001, "张三", 85.5f};
student_t student2 = {1002, "李四", 92.0f};

// 可以创建多个同类型的表实例
linx_hash_map_create_table("student_1", &student1);
linx_hash_map_create_table("student_2", &student2);

// 共享字段定义
linx_hash_map_add_field("student_1", "score", offsetof(student_t, score), sizeof(float), FIELD_TYPE_FLOAT);
linx_hash_map_add_field("student_2", "score", offsetof(student_t, score), sizeof(float), FIELD_TYPE_FLOAT);
```

### 场景3：动态基地址更新
```c
student_t *original_student = malloc(sizeof(student_t));
// ... 初始化数据 ...

// 创建表并设置初始基地址
linx_hash_map_create_table("student", original_student);

// 稍后需要重新分配内存
student_t *new_student = realloc(original_student, sizeof(student_t));

// 更新基地址
linx_hash_map_update_base_addr("student", new_student);

// 继续正常使用，系统会自动使用新的基地址
field_result_t field = linx_hash_map_get_field("student", "name");
char *name = (char *)linx_hash_map_get_table_value_ptr("student", &field);
```

## 最佳实践

### 1. 表名规范
- 使用有意义的表名，如结构体类型名
- 对于同类型多实例，使用后缀区分：`"student_1"`, `"student_2"`
- 避免使用特殊字符和空格

### 2. 基地址管理
- 在结构体创建时立即设置基地址
- 内存重新分配后及时更新基地址
- 结构体销毁前记得清理对应的表

### 3. 错误处理
```c
// 检查基地址更新是否成功
if (linx_hash_map_update_base_addr("student", new_addr) != 0) {
    fprintf(stderr, "Failed to update base address for table 'student'\n");
    // 处理错误
}

// 检查字段访问是否成功
field_result_t field = linx_hash_map_get_field("student", "name");
if (!field.found) {
    fprintf(stderr, "Field 'name' not found in table 'student'\n");
    // 处理错误
}
```

### 4. 内存管理
```c
// 获取表列表后记得释放
char **table_names;
size_t count;
if (linx_hash_map_list_tables(&table_names, &count) == 0) {
    // 使用表名列表
    for (size_t i = 0; i < count; i++) {
        printf("Table: %s\n", table_names[i]);
    }
    
    // 释放内存
    linx_hash_map_free_table_list(table_names, count);
}
```

## 编译和运行示例

```bash
# 编译库
make

# 编译示例程序
make examples

# 运行示例
./build/bin/example_multi_struct
```

## 注意事项

1. **线程安全**：当前实现不是线程安全的，多线程环境下需要外部同步
2. **内存管理**：调用者负责管理结构体实例的内存生命周期
3. **基地址有效性**：系统不会验证基地址的有效性，使用无效地址会导致程序崩溃
4. **表名唯一性**：同一个表名只能创建一次，重复创建会失败

## API参考

完整的API文档请参考 `linx_hash_map.h` 头文件中的函数声明和注释。