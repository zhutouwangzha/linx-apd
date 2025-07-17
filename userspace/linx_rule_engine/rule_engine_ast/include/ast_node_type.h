#ifndef __AST_NODE_TYPE_H__
#define __AST_NODE_TYPE_H__ 

typedef enum {
    AST_NODE_TYPE_NONE,
    AST_NODE_TYPE_INT,          /* 标识为整数 */
    AST_NODE_TYPE_FLOAT,        /* 标识为浮点数 */
    AST_NODE_TYPE_STRING,
    AST_NODE_TYPE_LIST,         /* 列表 */
    AST_NODE_TYPE_FIELD_NAME,   /* 如proc.pid */
    AST_NODE_TYPE_BIN_BOOL_OP,  /* 布尔二元操作符号 */
    AST_NODE_TYPE_BIN_NUM_OP,   /* 数值二元操作符号 */
    AST_NODE_TYPE_BIN_STR_OP,   /* 字符串二元操作符号 */
    AST_NODE_TYPE_BIN_LIST_OP,  /* 列表二元操作符号 */
    AST_NODE_TYPE_UN_OP,        /* 一元操作符 */
    AST_NODE_TYPE_MAX
} ast_node_type_t;

#endif /* __AST_NODE_TYPE_H__ */
