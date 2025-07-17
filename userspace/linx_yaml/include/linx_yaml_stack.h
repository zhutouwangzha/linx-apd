#ifndef __LINX_YAML_STACK_H__
#define __LINX_YAML_STACK_H__ 

#include "linx_yaml_node.h"

typedef struct {
    linx_yaml_node_t **stack;
    int size;
    int capacity;
    char *current_key;
    int in_sequence;
} linx_yaml_stack_t;

linx_yaml_stack_t *linx_yaml_stack_create(void);

void linx_yaml_stack_free(linx_yaml_stack_t *stack);

void linx_yaml_stack_push(linx_yaml_stack_t *stack, linx_yaml_node_t *node);

linx_yaml_node_t *linx_yaml_stack_pop(linx_yaml_stack_t *stack);

linx_yaml_node_t *linx_yaml_stack_top(linx_yaml_stack_t *stack);

#endif /* __LINX_YAML_STACK_H__ */