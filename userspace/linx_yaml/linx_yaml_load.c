#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "yaml.h"

#include "linx_yaml_load.h"

/**
 * @brief 检查指定文件是否存在
 * 
 * 该函数通过调用stat系统调用来检查指定路径的文件是否存在。
 * 如果文件存在且可访问，则返回成功状态；否则返回失败状态。
 * 
 * @param file_name 要检查的文件路径（字符串形式）
 * @return int 返回1表示文件存在，0表示文件不存在或不可访问
 */
static int file_exists(const char *file_name)
{
    struct stat buffer;
    /* 调用stat函数检查文件状态，返回值为0表示成功 */
    return (stat(file_name, &buffer) == 0);
}

/**
 * @brief 从YAML配置文件加载并解析内容，构建节点树
 * 
 * @param config_file YAML配置文件路径
 * @return linx_yaml_node_t* 成功返回根节点指针，失败返回NULL
 * 
 * 该函数执行以下操作：
 * 1. 检查并打开配置文件
 * 2. 初始化YAML解析器
 * 3. 使用状态栈处理YAML事件流
 * 4. 构建节点树结构
 * 5. 清理资源并返回结果
 */
linx_yaml_node_t *linx_yaml_load(const char *config_file)
{
    int error = 0, done = 0;
    yaml_parser_t parser;
    yaml_event_t event;
    linx_yaml_node_t *root = NULL;
    linx_yaml_stack_t *stack = linx_yaml_stack_create();
    FILE *file;
    
    /* 前置检查：确保文件存在且可访问 */
    if (!file_exists(config_file)) {
        return NULL;
    }

    /* 打开文件并初始化YAML解析器 */
    file = fopen(config_file, "rb");
    if (!file) {
        return NULL;
    }

    if (!yaml_parser_initialize(&parser)) {
        fclose(file);
        return NULL;
    }

    yaml_parser_set_input_file(&parser, file);

    /* 主解析循环：处理YAML事件流 */
    while (!done && !error) {
        if (!yaml_parser_parse(&parser, &event)) {
            fprintf(stderr, "解析错误: %s (行 %ld, 列 %ld)\n", 
                    parser.problem, parser.problem_mark.line + 1, 
                    parser.problem_mark.column + 1);
            error = 1;
            break;
        }

        /* 根据事件类型构建节点树 */
        switch (event.type) {
        case YAML_STREAM_START_EVENT:
            // 流开始事件无需处理
            break;
        case YAML_STREAM_END_EVENT: 
            // 流结束事件标记解析完成
            done = 1;
            break;
        case YAML_DOCUMENT_START_EVENT:
            // 文档开始事件无需处理
            break;
        case YAML_DOCUMENT_END_EVENT:
            // 文档结束事件无需处理
            break;
        case YAML_MAPPING_START_EVENT:
        {
            /* 处理映射类型节点创建 */
            linx_yaml_node_t *node = linx_yaml_node_create(LINX_YAML_NODE_TYPE_MAPPING, stack->current_key);

            if (stack->current_key) {
                free(stack->current_key);
                stack->current_key = NULL;
            }

            /* 将节点添加到树结构中 */
            if (stack->size > 0) {
                linx_yaml_node_t *parent = linx_yaml_stack_top(stack);
                linx_yaml_node_add_child(parent, node);
            } else if (!root) {
                root = node;
            }

            stack->in_sequence = 0;
            linx_yaml_stack_push(stack, node);
            break;
        }
        case YAML_SEQUENCE_START_EVENT:
        {
            /* 处理序列类型节点创建 */
            linx_yaml_node_t *node = linx_yaml_node_create(LINX_YAML_NODE_TYPE_SEQUENCE, stack->current_key);

            if (stack->current_key) {
                free(stack->current_key);
                stack->current_key = NULL;
            }

            /* 将节点添加到树结构中 */
            if (stack->size > 0) {
                linx_yaml_node_t *parent = linx_yaml_stack_top(stack);
                linx_yaml_node_add_child(parent, node);
            } else if (!root) {
                root = node;
            }

            stack->in_sequence = 1;
            linx_yaml_stack_push(stack, node);
            break;
        }
        case YAML_SCALAR_EVENT:
        {
            /* 处理标量值（键或值） */
            const char *value = (const char *)event.data.scalar.value;

            if (stack->in_sequence) {
                /* 序列中的标量值处理 */
                linx_yaml_node_t *node = linx_yaml_node_create(LINX_YAML_NODE_TYPE_SCALAR, NULL);
                node->value = strdup(value);
                node->is_sequence_item = 1;

                if (stack->size > 0) {
                    linx_yaml_node_t *parent = linx_yaml_stack_top(stack);
                    linx_yaml_node_add_child(parent, node);
                }
            } else if (stack->current_key) {
                /* 映射中的值处理 */
                linx_yaml_node_t *node = linx_yaml_node_create(LINX_YAML_NODE_TYPE_SCALAR, stack->current_key);
                node->value = strdup(value);

                if (stack->size > 0) {
                    linx_yaml_node_t *parent = linx_yaml_stack_top(stack);
                    linx_yaml_node_add_child(parent, node);
                }

                free(stack->current_key);
                stack->current_key = NULL;
            } else {
                /* 映射中的键处理 */
                stack->current_key = strdup(value);
            }

            break;
        }
        case YAML_MAPPING_END_EVENT:
            /* 映射结束事件处理 */
            linx_yaml_stack_pop(stack);
            break;
        case YAML_SEQUENCE_END_EVENT:
            /* 序列结束事件处理 */
            linx_yaml_stack_pop(stack);
            stack->in_sequence = 0;
            break;
        default:
            break;
        }

        yaml_event_delete(&event);
    }

    /* 清理资源 */
    if (stack->current_key) {
        free(stack->current_key);
    }

    linx_yaml_stack_free(stack);
    yaml_parser_delete(&parser);
    fclose(file);

    /* 错误处理：释放已分配的资源 */
    if (error) {
        if (root) {
            linx_yaml_node_free(root);
        }

        return NULL;
    }

    return root;
}
