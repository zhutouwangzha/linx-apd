#ifndef __RULE_MATCH_STRUCT_H__
#define __RULE_MATCH_STRUCT_H__ 

#include "rule_match_context.h"

typedef bool (*match_func_t)(void *context);

typedef struct {
    match_func_t func;
    match_context_type_t type;
    void *context;
} linx_rule_match_t;

#endif /* __RULE_MATCH_STRUCT_H__ */
