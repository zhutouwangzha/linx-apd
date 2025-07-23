# 动态基地址支持

本文档说明了如何使用新的API来支持多线程环境中的动态基地址。

## 背景

在原始设计中，`linx_hash_map_create_table`在创建表时就绑定了结构体的基地址。这在多线程环境中会造成问题，因为每个线程可能有自己的结构体实例。

## 解决方案

我们修改了设计，使得基地址可以在匹配时动态传入：

1. `field_result_t`现在存储偏移量而不是计算后的地址
2. 提供了新的API函数来支持动态基地址

## 新的API

### 对于rule_match：

```c
// 使用动态基地址进行匹配
bool linx_rule_engine_match_with_base(linx_rule_match_t *match, void *base_addr);

// 为match设置基地址（会递归设置所有子context）
void linx_rule_engine_match_set_base(linx_rule_match_t *match, void *base_addr);
```

### 对于output_match：

```c
// 使用动态基地址进行格式化
int linx_output_match_format_with_base(linx_output_match_t *match, void *base_addr, char *buffer, size_t buffer_size);
```

## 使用示例

### 单线程使用（向后兼容）

```c
// 创建表时可以传入基地址（向后兼容）
event_t evt;
linx_hash_map_create_table("evt", &evt);

// 使用原有API
linx_rule_match_t *match = ...;
bool result = linx_rule_engine_match(match);
```

### 多线程使用

```c
// 创建表时不传入基地址（传入NULL）
linx_hash_map_create_table("evt", NULL);

// 在每个线程中：
void thread_function(void *arg)
{
    // 每个线程有自己的事件实例
    event_t thread_evt;
    
    // 使用新的API，动态传入基地址
    linx_rule_match_t *match = get_shared_match();
    bool result = linx_rule_engine_match_with_base(match, &thread_evt);
    
    // 格式化输出也使用动态基地址
    linx_output_match_t *output = get_shared_output();
    char buffer[1024];
    linx_output_match_format_with_base(output, &thread_evt, buffer, sizeof(buffer));
}
```

## 注意事项

1. 新的API要求传入的基地址不能为NULL
2. 使用动态基地址时，确保基地址指向的结构体在匹配期间保持有效
3. `linx_rule_engine_match_set_base`会递归设置所有子context的基地址
4. 原有的API仍然可用，保持向后兼容性

## 迁移建议

如果您的应用需要在多线程环境中使用规则引擎：

1. 创建表时传入NULL作为基地址：`linx_hash_map_create_table("evt", NULL)`
2. 使用新的`*_with_base`API函数
3. 确保每个线程传入正确的基地址

这样可以确保每个线程使用自己的数据实例，避免线程间的数据冲突。