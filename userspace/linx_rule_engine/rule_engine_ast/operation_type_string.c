#include <string.h>

#include "operation_type.h"

/**
 * 一元操作符
*/
const char *g_unary_ops[UNARY_OP_MAX] = {
    "exists",
    "not"
};

/**
 * 二元数字操作符
*/
const char *g_binary_num_ops[BINARY_NUM_OP_MAX] = {
    ">",
    ">=",
    "<",
    "<="
};

/**
 * 二元字符串操作符
*/
const char *g_binary_str_ops[BINARY_STR_OP_MAX] = {
    "==",
    "=",
    "!=",
    "glob ",
    "iglob ",
    "contains ",
    "icontains ",
    "bcontains ",
    "startswith ",
    "bstartswith ",
    "endswith ",
    "regex "
};

/**
 * 二元列表操作符
*/
const char *g_binary_list_ops[BINARY_LIST_OP_MAX] = {
    "intersects",
    "in",
    "pmatch"
};

/**
 * 二元布尔操作符
*/
const char *g_binary_bool_ops[BINARY_BOOL_OP_MAX] = {
    "or",
    "and"
};

unary_op_type_t op_str_to_unary_op(const char *str)
{
    unary_op_type_t op = -1;

    for (int i = 0; i < UNARY_OP_MAX; i++) {
        if (strcmp(str, g_unary_ops[i]) == 0) {
            op = i;
            break;
        }
    }

    return op;
}

binary_num_op_type_t op_str_to_binary_num_op(const char *str)
{
    binary_num_op_type_t op = -1; 

    for (int i = 0; i < BINARY_NUM_OP_MAX; i++) {
        if (strcmp(str, g_binary_num_ops[i]) == 0) {
            op = i;
            break;
        }
    }

    return op;
}

binary_str_op_type_t op_str_to_binary_str_op(const char *str)
{
    binary_str_op_type_t op = -1; 

    for (int i = 0; i < BINARY_STR_OP_MAX; i++) {
        if (strcmp(str, g_binary_str_ops[i]) == 0) {
            op = i;
            break;
        }
    }

    return op;
}

binary_list_op_type_t op_str_to_binary_list_op(const char *str)
{
    binary_list_op_type_t op = -1;

    for (int i = 0; i < BINARY_LIST_OP_MAX; i++) {
        if (strcmp(str, g_binary_list_ops[i]) == 0) {
            op = i;
            break;
        }
    }

    return op;
}

binary_bool_op_type_t op_str_to_binary_bool_op(const char *str)
{
    binary_bool_op_type_t op = -1;

    for (int i = 0; i < BINARY_BOOL_OP_MAX; i++) {
        if (strcmp(str, g_binary_bool_ops[i]) == 0) {
            op = i;
            break;
        }
    }

    return op; 
}

int op_str_to_op_type(const char *str)
{
    int op = -1;

    op = op_str_to_unary_op(str);
    op = (op != -1) ? op : (int)op_str_to_binary_num_op(str);
    op = (op != -1) ? op : (int)op_str_to_binary_str_op(str);
    op = (op != -1) ? op : (int)op_str_to_binary_list_op(str);
    op = (op != -1) ? op : (int)op_str_to_binary_bool_op(str);

    return op;
}
