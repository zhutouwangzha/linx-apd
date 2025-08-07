#include <stdlib.h>
#include <string.h>

#include "ast_node.h"

ast_node_t *ast_node_create(ast_node_type_t type)
{
    ast_node_t *node = malloc(sizeof(ast_node_t)); 
    if (!node) {
        return NULL;
    }

    memset(node, 0 , sizeof(ast_node_t));

    node->type = type;

    return node;
}

void ast_node_destroy(ast_node_t *node)
{
    if (!node) {
        return;
    }

    switch (node->type) {
    case AST_NODE_TYPE_STRING:
        free(node->data.string_value);
        node->data.string_value = NULL;
        break;
    case AST_NODE_TYPE_FIELD_NAME:
        free(node->data.field.name);
        node->data.field.name = NULL;
        break;
    case AST_NODE_TYPE_BIN_NUM_OP:
    case AST_NODE_TYPE_BIN_STR_OP:
        ast_node_destroy(node->data.binary.left);
        ast_node_destroy(node->data.binary.right);
        break;
    case AST_NODE_TYPE_UN_OP:
        ast_node_destroy(node->data.unary.operand);
        break;
    case AST_NODE_TYPE_LIST:
        for (int i = 0; i < node->data.list.count; i++) {
            ast_node_destroy(node->data.list.items[i]);
        }

        free(node->data.list.items);
        node->data.list.items = NULL;
        break;
    case AST_NODE_TYPE_FIELD_TRANSFORMER:
        ast_node_destroy(node->data.field_transformer.operand);
        break;
    default:
        break;
    }

    free(node);
    node = NULL;
}

ast_node_t *ast_node_create_number(long long value)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_INT);

    node->data.number_value.int_value = value;

    return node;
}

ast_node_t *ast_node_create_float(long double value)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_FLOAT);

    node->data.number_value.double_value = value;

    return node;
}


ast_node_t *ast_node_create_string(char *str)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_STRING);

    node->data.string_value = strdup(str);

    return node;
}

ast_node_t *ast_node_create_field_name(char *name, char *arg)
{
    (void)arg;
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_FIELD_NAME);

    node->data.field.name = strdup(name);

    return node;
}

ast_node_t *ast_node_create_list(void)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_LIST);
    
    node->data.list.items = NULL;
    node->data.list.count = 0;

    return node;
}

void add_item_to_list(ast_node_t *list, ast_node_t *item)
{
    list->data.list.count++;

    list->data.list.items = (ast_node_t **)realloc(
        list->data.list.items, 
        list->data.list.count * sizeof(ast_node_t *)
    );

    list->data.list.items[list->data.list.count - 1] = item;
}

ast_node_t *ast_node_create_binary_num(binary_num_op_type_t op, ast_node_t *left, ast_node_t *right)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_BIN_NUM_OP);

    node->data.binary.op.num_op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;

    return node;
}

ast_node_t *ast_node_create_binary_str(binary_str_op_type_t op, ast_node_t *left, ast_node_t *right)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_BIN_STR_OP);

    node->data.binary.op.str_op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;

    return node;
}

ast_node_t *ast_node_create_binary_list(binary_list_op_type_t op, ast_node_t *left, ast_node_t *right)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_BIN_LIST_OP);

    node->data.binary.op.list_op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;

    return node;
}

ast_node_t *ast_node_create_binary_bool(binary_bool_op_type_t op, ast_node_t *left, ast_node_t *right)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_BIN_BOOL_OP);

    node->data.binary.op.bool_op = op;
    node->data.binary.left = left;
    node->data.binary.right = right;

    return node;
}

ast_node_t *ast_node_create_unary(unary_op_type_t op, ast_node_t *operand)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_UN_OP);

    node->data.unary.unary_op = op;
    node->data.unary.operand = operand;

    return node;
}

ast_node_t *ast_node_create_field_transformer(field_transformer_type_t op, ast_node_t *operand)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_FIELD_TRANSFORMER);

    node->data.field_transformer.type.type = op;
    node->data.field_transformer.operand = operand;

    return node;
}

ast_node_t *ast_node_create_field_transformer_val(field_transformer_val_t op, ast_node_t *operand)
{
    ast_node_t *node = ast_node_create(AST_NODE_TYPE_FIELD_TRANSFORMER_VAL);

    node->data.field_transformer.type.val = op;
    node->data.field_transformer.operand = operand;

    return node;
}
