#ifndef __OPERATION_TYPE_H__
#define __OPERATION_TYPE_H__ 

typedef enum {
    UNARY_OP_EXISTS,
    UNARY_OP_NOT,
    UNARY_OP_MAX
} unary_op_type_t;

typedef enum {
    BINARY_NUM_OP_GT,       /* > */
    BINARY_NUM_OP_GE,       /* >= */
    BINARY_NUM_OP_LT,       /* < */
    BINARY_NUM_OP_LE,       /* <= */
    BINARY_NUM_OP_MAX
} binary_num_op_type_t;

typedef enum {
    BINARY_STR_OP_EQ,       /* == */
    BINARY_STR_OP_ASSIGN,   /* = */
    BINARY_STR_OP_NE,       /* != */
    BINARY_STR_OP_GLOB,     /* glob */
    BINARY_STR_OP_IGLOB,
    BINARY_STR_OP_CONTAINS,
    BINARY_STR_OP_ICONTAINS,
    BINARY_STR_OP_BCONTAINS,
    BINARY_STR_OP_STARTSWITH,
    BINARY_STR_OP_BSTARTSWITH,
    BINARY_STR_OP_ENDSWITH,
    BINARY_STR_OP_REGEX,
    BINARY_STR_OP_MAX
} binary_str_op_type_t;

typedef enum {
    BINARY_LIST_OP_INTERSECTS,
    BINARY_LIST_OP_IN,
    BINARY_LIST_OP_PMATCH,
    BINARY_LIST_OP_MAX
} binary_list_op_type_t;

typedef enum {
    BINARY_BOOL_OP_OR,
    BINARY_BOOL_OP_AND,
    BINARY_BOOL_OP_MAX
} binary_bool_op_type_t;

extern const char *g_unary_ops[UNARY_OP_MAX];

extern const char *g_binary_num_ops[BINARY_NUM_OP_MAX];

extern const char *g_binary_str_ops[BINARY_STR_OP_MAX];

extern const char *g_binary_list_ops[BINARY_LIST_OP_MAX];

extern const char *g_binary_bool_ops[BINARY_BOOL_OP_MAX];

unary_op_type_t op_str_to_unary_op(const char *str);

binary_num_op_type_t op_str_to_binary_num_op(const char *str);

binary_str_op_type_t op_str_to_binary_str_op(const char *str);

binary_list_op_type_t op_str_to_binary_list_op(const char *str);

binary_bool_op_type_t op_str_to_binary_bool_op(const char *str);

int op_str_to_op_type(const char *str);

#endif /* __OPERATION_TYPE_H__ */
