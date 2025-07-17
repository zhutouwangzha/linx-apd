#ifndef __AST_NODE_H__
#define __AST_NODE_H__

#include "ast_node_type.h"
#include "operation_type.h"

typedef struct ast_node_s {
    ast_node_type_t type;
    union {
        union {
            long long int_value;
            long double double_value;
        } number_value;
        char *string_value;
        char *field_name;

        /* 列表 */
        struct {
            struct ast_node_s **items;
            int count;
        } list;

        /* 二元操作 */
        struct {
            struct ast_node_s *left;
            struct ast_node_s *right;
            union {
                binary_num_op_type_t num_op;
                binary_str_op_type_t str_op;
                binary_list_op_type_t list_op;
                binary_bool_op_type_t bool_op;
            } op;
        } binary;

        /* 一元操作 */
        struct {
            struct ast_node_s *operand;
            unary_op_type_t unary_op;
        } unary;
    } data;
} ast_node_t;

ast_node_t *ast_node_create(ast_node_type_t type);

void ast_node_destroy(ast_node_t *node);

ast_node_t *ast_node_create_number(long long value);

ast_node_t *ast_node_create_float(long double value);

ast_node_t *ast_node_create_string(char *str);

ast_node_t *ast_node_create_field_name(char *name);

ast_node_t *ast_node_create_list(void);

void add_item_to_list(ast_node_t *list, ast_node_t *item);

ast_node_t *ast_node_create_binary_num(binary_num_op_type_t op, ast_node_t *left, ast_node_t *right);

ast_node_t *ast_node_create_binary_str(binary_str_op_type_t op, ast_node_t *left, ast_node_t *right);

ast_node_t *ast_node_create_binary_list(binary_list_op_type_t op, ast_node_t *left, ast_node_t *right);

ast_node_t *ast_node_create_binary_bool(binary_bool_op_type_t op, ast_node_t *left, ast_node_t *right);

ast_node_t *ast_node_create_unary(unary_op_type_t op, ast_node_t *operand);

#endif /* __AST_NODE_H__ */
