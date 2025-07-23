#ifndef __RULE_MATCH_CONTEXT_H__
#define __RULE_MATCH_CONTEXT_H__ 

#include "linx_hash_map.h"

typedef enum {
    MATCH_CONTEXT_STR,
    MATCH_CONTEXT_NUM,
    MATCH_CONTEXT_LOGIC,
    MATCH_CONTEXT_MAX
} match_context_type_t;

typedef struct {
    field_result_t field;
    const char *str;
    size_t str_len;
    void *base_addr;    /* 动态基地址 */
} str_context_t;

typedef struct {
    field_result_t field;
    union {
        long long int_val;
        long double double_val;
    } number;
    void *base_addr;    /* 动态基地址 */
} num_context_t;

typedef struct {
    void *left;         /* 指向 linx_rule_match_t */
    void *right;        /* 指向 linx_rule_match_t */
} logic_context_t;

#endif /* __RUEL_MATCH_CONTEXT_H__ */
