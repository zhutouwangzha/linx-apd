#include <stdlib.h>
#include <stdio.h>

#include "linx_rule_engine_set.h"

static linx_rule_set_t *rule_set = NULL;

int linx_rule_set_init(void)
{
    rule_set = malloc(sizeof(linx_rule_set_t));
    if (rule_set == NULL) {
        return -1;
    }

    rule_set->capacity = 0;
    rule_set->size = 0;

    rule_set->data.rules = NULL;
    rule_set->data.matches = NULL;
    rule_set->data.outputs = NULL;

    return 0;
}

void linx_rule_set_deinit(void)
{
    if (!rule_set) {
        return;
    }

    for (size_t i = 0; i < rule_set->size; i++) {
        free(rule_set->data.rules[i]);
        free(rule_set->data.matches[i]);
        free(rule_set->data.outputs[i]);
    }

    free(rule_set);
}

linx_rule_set_t *linx_rule_set_get(void)
{
    return rule_set;
}

static int linx_rule_set_resize(void)
{
    size_t new_capacity;
    linx_rule_t **new_rules;
    linx_rule_match_t **new_matches;
    linx_output_match_t **new_outputs;

    new_capacity = rule_set->capacity == 0 ? 16 : rule_set->capacity * 2;

    new_rules = realloc(rule_set->data.rules, new_capacity * sizeof(linx_rule_t *));
    if (new_rules == NULL) {
        return -1;
    }

    new_matches = realloc(rule_set->data.matches, new_capacity * sizeof(linx_rule_match_t *));
    if (new_matches == NULL) {
        free(new_rules);
        return -1;
    }

    new_outputs = realloc(rule_set->data.outputs, new_capacity * sizeof(linx_output_match_t *));
    if (new_outputs == NULL) {
        free(new_rules);
        free(new_matches);
        return -1;
    }

    rule_set->capacity = new_capacity;
    rule_set->data.rules = new_rules;
    rule_set->data.matches = new_matches;
    rule_set->data.outputs = new_outputs;

    return 0;
}

int linx_rule_set_add(linx_rule_t *rule, linx_rule_match_t *match, linx_output_match_t *output)
{
    if (rule_set == NULL || rule == NULL || 
        match == NULL || output == NULL) 
    {
        return -1;
    }

    if (rule_set->size >= rule_set->capacity) {
        if (linx_rule_set_resize()) {
            return -1;
        }
    }

    rule_set->data.rules[rule_set->size] = rule;
    rule_set->data.matches[rule_set->size] = match;
    rule_set->data.outputs[rule_set->size] = output;

    rule_set->size++;

    return 0;
}

bool linx_rule_set_match_rule(void)
{
    bool match = false;

    if (rule_set == NULL) {
        return false;
    }

    for (size_t i = 0; i < rule_set->size; i++) {
        if (rule_set->data.matches[i]) {
            if (rule_set->data.matches[i]->func(rule_set->data.matches[i]->context)) {
                match = true;
                

                /**
                 * 这里有一个yaml配置可以控制匹配到规则后是否继续匹配后面的规则
                 * 计划在后续添加 
                 * 这里还要添加一个匹配成功然后输出消息的逻辑
                */
                break;
            }
        }
    }

    return match;
}
