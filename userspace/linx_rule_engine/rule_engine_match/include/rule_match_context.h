#ifndef __RULE_MATCH_CONTEXT_H__
#define __RULE_MATCH_CONTEXT_H__ 

#include "linx_hash_map.h"

typedef char *(*transformer_func_t)(char *);

typedef enum {
    MATCH_CONTEXT_STR,
    MATCH_CONTEXT_NUM,
    MATCH_CONTEXT_LIST,
    MATCH_CONTEXT_UNARY,
    MATCH_CONTEXT_LOGIC,
    MATCH_CONTEXT_MAX
} match_context_type_t;

typedef struct {
    field_result_t field;
    char *str;
    size_t str_len;
    transformer_func_t *func;
    size_t func_num;
} str_context_t;

typedef struct {
    field_result_t field;
    union {
        long long int_val;
        long double double_val;
    } number;
} num_context_t;

typedef struct {
    field_result_t field;
    char **list;
    size_t *list_len;
    size_t list_count;
} list_context_t;

typedef struct {
    void *operand;      /* 指向 linx_rule_match_t */
    char *op;           /* 只在exitst使用 */
} unary_context_t;

typedef struct {
    void *left;         /* 指向 linx_rule_match_t */
    void *right;        /* 指向 linx_rule_match_t */
} logic_context_t;

#endif /* __RUEL_MATCH_CONTEXT_H__ */
