#include <stdlib.h>

#include "linx_yaml_stack.h"

/**
 * @brief 创建一个新的YAML解析栈结构
 *
 * 该函数负责初始化并返回一个新的YAML解析栈，用于在解析YAML文档时维护状态。
 * 栈的初始容量为16个节点指针，当前大小设为0，并初始化当前键为NULL。
 *
 * @return linx_yaml_stack_t* 返回新创建的栈结构指针
 *                          (需要调用者负责后续的内存释放)
 */
linx_yaml_stack_t *linx_yaml_stack_create(void)
{
    /* 分配栈结构内存并初始化基本字段 */
    linx_yaml_stack_t *stack = malloc(sizeof(linx_yaml_stack_t));
    if (!stack) {
        return NULL;
    }

    stack->capacity = 16;
    stack->size = 0;
    
    /* 分配节点指针数组内存 */
    stack->stack = malloc(stack->capacity * sizeof(linx_yaml_node_t *));
    if (!stack->stack) {
        free(stack);
        return NULL;
    }

    stack->current_key = NULL;
    
    return stack;
}

/**
 * @brief 释放YAML栈结构及其所有关联内存
 * 
 * 该函数负责安全释放与YAML栈相关的所有动态分配内存，包括：
 * 1. 栈内部的节点指针数组(stack->stack)
 * 2. 当前存储的键名(current_key)
 * 3. 栈结构体本身
 * 
 * @param stack 指向要释放的linx_yaml_stack_t结构体指针。
 *              如果传入NULL，函数将直接返回。
 */
void linx_yaml_stack_free(linx_yaml_stack_t *stack)
{
    /* 安全检查：如果栈指针为NULL则直接返回 */
    if (!stack) {
        return;
    }

    /* 释放栈内部的节点指针数组 */
    if (stack->stack) {
        free(stack->stack);
        stack->stack = NULL;
    }
    
    /* 释放当前键名字符串（如果已分配） */
    if (stack->current_key) {
        free(stack->current_key);
        stack->current_key = NULL;
    }

    /* 最后释放栈结构体本身 */
    free(stack);
    stack = NULL;
}

/**
 * @brief 将节点压入YAML解析栈中
 *
 * 该函数用于在YAML解析过程中将节点压入动态增长的栈结构中。如果栈已满，
 * 函数会自动扩容（容量翻倍）后再压入节点。
 *
 * @param stack 指向YAML解析栈的指针，栈结构包含当前大小、容量和节点指针数组
 * @param node 要压入栈的YAML节点指针
 */
int linx_yaml_stack_push(linx_yaml_stack_t *stack, linx_yaml_node_t *node)
{
    int new_capacity;
    linx_yaml_node_t **new_stack;

    /* 检查栈容量，不足时进行扩容 */
    if (stack->size >= stack->capacity) {
        new_capacity = stack->capacity * 2;
        new_stack = realloc(stack->stack, new_capacity * sizeof(linx_yaml_node_t *));
        if (!new_stack) {
            return -1;
        }
        
        stack->capacity = new_capacity;
        stack->stack = new_stack;
    }

    /* 将节点压入栈顶并更新栈大小 */
    stack->stack[stack->size++] = node;

    return 0;
}

linx_yaml_node_t *linx_yaml_stack_pop(linx_yaml_stack_t *stack)
{
    if (stack->size <= 0) {
        return NULL;
    }

    return stack->stack[--stack->size];
}

linx_yaml_node_t *linx_yaml_stack_top(linx_yaml_stack_t *stack)
{
    if (stack->size <= 0) {
        return NULL;
    }

    return stack->stack[stack->size - 1];
}
