#include <string.h>
#include <stdlib.h> 

#include "linx_yaml_node.h"

/**
 * @brief 创建一个新的YAML节点
 * 
 * 该函数用于创建一个新的YAML节点结构体，并初始化其成员变量。
 * 节点可以是YAML文档中的任何类型（如映射、序列、标量等）。
 * 
 * @param type 节点的类型，由linx_yaml_node_type_t枚举定义
 * @param key 节点的键名，如果是序列中的项或根节点可以为NULL
 * @return linx_yaml_node_t* 返回新创建的节点指针，失败时返回NULL
 */
linx_yaml_node_t *linx_yaml_node_create(linx_yaml_node_type_t type, const char *key)
{
    /* 分配节点内存并检查是否成功 */
    linx_yaml_node_t *node = malloc(sizeof(linx_yaml_node_t));
    if (!node) {
        return NULL;
    }

    /* 初始化节点成员 */
    node->type = type;
    /* 如果key不为NULL则复制字符串，否则设为NULL */
    node->key = key ? strdup(key) : NULL;
    /* 初始化其他成员为默认值 */
    node->value = NULL;
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    node->is_sequence_item = 0;

    return node;
}

/**
 * @brief 释放YAML节点及其所有子节点的内存（递归释放）
 * 
 * 该函数会释放与给定YAML节点相关的所有内存，包括：
 * 1. 节点的键名(key)和值(value)字符串
 * 2. 所有子节点（递归释放）
 * 3. 子节点指针数组
 * 4. 节点结构体本身
 * 
 * @param node 要释放的YAML节点指针。如果为NULL则直接返回。
 */
void linx_yaml_node_free(linx_yaml_node_t *node)
{
    // 空指针检查
    if (!node) {
        return;
    }

    // 释放键名和值的字符串内存
    if (node->key) {
        free(node->key);
        node->key = NULL;
    }

    if (node->value) {
        free(node->value);
        node->value = NULL;
    }

    // 递归释放所有子节点
    for (int i = 0; i < node->child_count; i++) {
        linx_yaml_node_free(node->children[i]);
    }

    // 释放子节点指针数组和节点结构体
    free(node->children);
    node->children = NULL;


    free(node);
    node = NULL;
}

/**
 * @brief 向父节点添加一个子节点
 *
 * 该函数将一个子节点添加到父节点的子节点列表中。如果父节点没有子节点列表，
 * 则初始化一个初始容量为4的列表。如果当前容量不足，则动态扩容为原来的两倍。
 *
 * @param parent 父节点指针，如果为NULL则直接返回
 * @param child 要添加的子节点指针，如果为NULL则直接返回
 */
int linx_yaml_node_add_child(linx_yaml_node_t *parent, linx_yaml_node_t *child)
{
    int new_capacity;
    linx_yaml_node_t **new_children;

    // 检查输入参数有效性
    if (!parent || !child) {
        return -1;
    }

    // 检查并处理容量不足的情况
    if (parent->child_count >= parent->child_capacity) {
        new_capacity = parent->child_capacity ? (parent->child_capacity * 2) : 4;
        new_children = realloc(parent->children, new_capacity * sizeof(linx_yaml_node_t *));
        if (!new_children) {
            return -1;
        }
        
        parent->child_capacity = new_capacity;
        parent->children = new_children;
    }

    // 添加子节点并更新计数
    parent->children[parent->child_count++] = child;

    return 0;
}

/**
 * 在YAML映射节点中查找指定键的子节点
 *
 * 该函数遍历给定YAML映射节点的所有子节点，查找与指定键匹配的子节点。
 * 如果找到则返回该子节点，否则返回NULL。
 *
 * @param node 要搜索的YAML节点，必须是映射类型(LINX_YAML_NODE_TYPE_MAPPING)
 * @param key  要查找的键名
 * @return     找到的匹配子节点，若未找到或输入无效则返回NULL
 */
linx_yaml_node_t *linx_yaml_node_find_by_key(linx_yaml_node_t *node, const char *key)
{
    /* 检查输入节点是否有效且为映射类型 */
    if (!node || node->type != LINX_YAML_NODE_TYPE_MAPPING) {
        return NULL;
    }

    /* 遍历所有子节点查找匹配键 */
    for (int i = 0; i < node->child_count; i++) {
        linx_yaml_node_t *child = node->children[i];
        if (child->key && strcmp(child->key, key) == 0) {
            return child;
        }
    }

    return NULL;
}

/**
 * @brief 通过索引获取YAML序列节点的子节点
 * 
 * 从YAML序列节点中获取指定索引位置的子节点。该函数会先进行参数校验，
 * 确保输入节点是有效的序列节点且索引值在合法范围内。
 * 
 * @param node 父节点指针，必须是LINX_YAML_NODE_TYPE_SEQUENCE类型的序列节点
 * @param index 要获取的子节点索引，必须为非负数且小于子节点数量
 * 
 * @return 成功时返回对应的子节点指针，失败时返回NULL。失败情况包括：
 *         - 输入节点为空
 *         - 节点类型不是序列节点
 *         - 索引越界(小于0或大于等于子节点数量)
 */
linx_yaml_node_t *linx_yaml_node_sequence_by_index(linx_yaml_node_t *node, int index)
{
    if (!node || node->type != LINX_YAML_NODE_TYPE_SEQUENCE) {
        return NULL;
    }

    if (index < 0 || index >= node->child_count) {
        return NULL;
    }

    return node->children[index];
}
