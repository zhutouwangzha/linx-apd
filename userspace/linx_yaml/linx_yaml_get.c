#include <stdlib.h>
#include <string.h>

#include "linx_yaml_node.h"

/**
 * @brief 根据路径查找YAML节点
 * 
 * 从给定的YAML根节点开始，按照路径查找对应的子节点。路径使用点号(.)分隔，
 * 可以包含映射(Mapping)键名或序列(Sequence)索引。
 * 
 * @param root YAML树的根节点指针，如果为NULL则直接返回NULL
 * @param path 查找路径字符串，格式如"key1.0.key2"，如果为NULL则直接返回NULL
 * @return linx_yaml_node_t* 查找到的节点指针，如果路径无效或节点不存在则返回NULL
 */
linx_yaml_node_t *linx_yaml_get_node_by_path(linx_yaml_node_t *root, const char *path)
{
    /* 检查输入参数有效性 */
    if (!root || !path) {
        return NULL;
    }

    /* 复制路径字符串用于分割处理(因为strtok会修改原字符串) */
    char *path_copy = strdup(path);
    char *saveptr;
    char *token = strtok_r(path_copy, ".", &saveptr);
    linx_yaml_node_t *current = root;

    /* 遍历路径的每个部分 */
    while (token && current) {
        /* 当前节点是映射类型时，通过键名查找 */
        if (current->type == LINX_YAML_NODE_TYPE_MAPPING) {
            current = linx_yaml_node_find_by_key(current, token);
        } 
        /* 当前节点是序列类型时，处理数字索引或特殊查找 */
        else if (current->type == LINX_YAML_NODE_TYPE_SEQUENCE) {
            char *endptr;
            long index = strtol(token, &endptr, 10);

            /* 如果路径部分是纯数字，作为索引处理 */
            if (*endptr == '\0') {
                current = linx_yaml_node_sequence_by_index(current, (int)index);
            } 
            /* 否则尝试在序列中查找包含该键名的映射节点 */
            else {
                for (int i = 0; i < current->child_count; i++) {
                    linx_yaml_node_t *child = current->children[i];

                    if (child->type == LINX_YAML_NODE_TYPE_MAPPING) {
                        linx_yaml_node_t *found = linx_yaml_node_find_by_key(current, token);
                        if (found) {
                            current = found;
                            break;
                        }
                    }
                }
                current = NULL;
            }
        } 
        /* 其他节点类型无法继续查找 */
        else {
            current = NULL;
        }

        /* 获取路径的下一部分 */
        token = strtok_r(NULL, ".", &saveptr);
    }
    
    /* 释放复制的路径字符串 */
    free(path_copy);
    return current;
}

/**
 * @brief 从YAML节点树中获取指定路径的字符串值
 *
 * 该函数通过给定的路径从YAML节点树中查找对应的节点，如果节点存在且为标量类型，
 * 则返回其字符串值；否则返回默认值。
 *
 * @param root YAML节点树的根节点
 * @param path 要查找的节点路径
 * @param default_value 当节点不存在或类型不匹配时返回的默认值
 * @return const char* 成功则返回节点字符串值，否则返回默认值
 */
const char *linx_yaml_get_string(linx_yaml_node_t *root, const char *path, const char *default_value)
{
    /* 根据路径查找对应的YAML节点 */
    linx_yaml_node_t *node = linx_yaml_get_node_by_path(root, path);

    /* 检查节点是否存在且为标量类型 */
    if (node && node->type == LINX_YAML_NODE_TYPE_SCALAR) {
        return node->value;
    }

    /* 节点不存在或类型不匹配时返回默认值 */
    return default_value;
}

/**
 * @brief 从YAML节点中获取指定路径的整数值
 *
 * 该函数通过路径查找YAML节点，并尝试将其值转换为整数。如果节点不存在、类型不匹配
 * 或转换失败，则返回默认值。
 *
 * @param root         YAML文档的根节点指针
 * @param path         要查找的节点路径（以'/'分隔的层级结构）
 * @param default_value 当查找或转换失败时返回的默认值
 * @return int         成功时返回转换后的整数值，失败时返回default_value
 */
int linx_yaml_get_int(linx_yaml_node_t *root, const char *path, int default_value)
{
    /* 通过路径查找目标节点 */
    linx_yaml_node_t *node = linx_yaml_get_node_by_path(root, path);

    /* 检查节点有效性并尝试转换值 */
    if (node && node->type == LINX_YAML_NODE_TYPE_SCALAR) {
        char *endptr;
        long value = strtol(node->value, &endptr, 10);
        
        /* 验证字符串是否完全转换为数字 */
        if (*endptr == '\0') {
            return (int)value;
        }
    }

    /* 所有失败情况返回默认值 */
    return default_value;
}

/**
 * @brief 从YAML节点中获取布尔值
 * 
 * 根据指定的路径从YAML节点树中查找对应节点，并尝试将其值解析为布尔类型。
 * 支持字符串形式的"true"/"yes"和"false"/"no"，也支持数字形式的0/1。
 * 如果解析失败则返回默认值。
 * 
 * @param root YAML节点树的根节点
 * @param path 要查找的节点路径
 * @param default_value 解析失败时返回的默认值
 * @return int 解析成功的布尔值(1/0)或默认值
 */
int linx_yaml_get_bool(linx_yaml_node_t *root, const char *path, int default_value)
{
    // 根据路径查找对应的YAML节点
    linx_yaml_node_t *node = linx_yaml_get_node_by_path(root, path);
    
    // 仅处理存在且为标量类型的节点
    if (node && node->type == LINX_YAML_NODE_TYPE_SCALAR) {
        // 检查是否为字符串形式的true/yes
        if (strcmp(node->value, "true") == 0 ||
            strcmp(node->value, "yes") == 0) 
        {
            return 1;
        }
        
        // 检查是否为字符串形式的false/no
        if (strcmp(node->value, "false") == 0 ||
            strcmp(node->value, "no") == 0) 
        {
            return 0;
        }

        // 尝试解析为数字形式(非零为true，零为false)
        char *endptr;
        long value = strtol(node->value, &endptr, 10);
        if (*endptr == '\0') {
            return value != 0;
        }
    }

    // 所有解析尝试失败时返回默认值
    return default_value;
}

/**
 * @brief 获取YAML序列节点的长度
 * 
 * 根据给定的路径查找YAML节点，如果该节点存在且为序列类型（SEQUENCE），
 * 则返回其子节点数量（即序列长度）；否则返回0。
 * 
 * @param root  YAML文档的根节点指针
 * @param path  需要查找的节点路径字符串
 * @return int  序列长度（存在且为序列类型时），否则返回0
 */
int linx_yaml_get_sequence_length(linx_yaml_node_t *root, const char *path)
{
    /* 通过路径查找目标节点 */
    linx_yaml_node_t *node = linx_yaml_get_node_by_path(root, path);

    /* 检查节点是否存在且为序列类型 */
    if (node && node->type == LINX_YAML_NODE_TYPE_SEQUENCE) {
        return node->child_count;
    }

    return 0;
}
