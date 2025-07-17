#ifndef __LINX_YAML_NODE_H__
#define __LINX_YAML_NODE_H__ 

typedef enum {
    LINX_YAML_NODE_TYPE_SCALAR,
    LINX_YAML_NODE_TYPE_MAPPING,
    LINX_YAML_NODE_TYPE_SEQUENCE
} linx_yaml_node_type_t;

typedef struct linx_yaml_node {
    char *key;
    char *value;
    linx_yaml_node_type_t type;

    struct linx_yaml_node **children;
    int child_count;
    int child_capacity;

    /* 序列项的特殊处理（无键名） */
    int is_sequence_item;
} linx_yaml_node_t;

linx_yaml_node_t *linx_yaml_node_create(linx_yaml_node_type_t type, const char *key);

void linx_yaml_node_free(linx_yaml_node_t *node);

void linx_yaml_node_add_child(linx_yaml_node_t *parent, linx_yaml_node_t *child);

linx_yaml_node_t *linx_yaml_node_find_by_key(linx_yaml_node_t *node, const char *key);

linx_yaml_node_t *linx_yaml_node_sequence_by_index(linx_yaml_node_t *node, int index);

#endif /* __LINX_YAML_NODE_H__ */