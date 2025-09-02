#ifndef __LINX_RULE_ENGINE_SET_H__
#define __LINX_RULE_ENGINE_SET_H__ 

#include <stddef.h>
#include <stdbool.h>

#include "linx_rule_engine_load.h"
#include "linx_rule_engine_match.h"
#include "linx_event_get.h"

typedef struct {
    struct {
        linx_rule_t **rules;
        linx_rule_match_t **matches;
        linx_output_match_t **outputs;
    } data;

    size_t size;
    size_t capacity;
} linx_rule_set_t;

int linx_rule_set_init(void);

void linx_rule_set_deinit(void);

linx_rule_set_t *linx_rule_set_get(void);

int linx_rule_set_add(linx_rule_t *rule, linx_rule_match_t *match, linx_output_match_t *output);

bool linx_rule_set_match_rule(void);

/* 多线程版本的规则匹配函数 */
bool linx_rule_set_match_rule_mt(linx_event_t *event, int64_t fd);

#endif /* __LINX_RULE_ENGINE_SET_H__ */
