#include <string.h>
#include <stdio.h>

#include "match_func.h"
#include "match_struct.h"
#include "match_context.h"
#include "linx_hash_map.h"

bool and_matcher(void *context)
{
    logic_context_t *ctx = (logic_context_t *)context;
    linx_rule_match_t *left = (linx_rule_match_t *)ctx->left;
    linx_rule_match_t *right = (linx_rule_match_t *)ctx->right;
    bool result;

    result = left->func(left->context);
    if (!result) {
        return result;
    }

    return result && right->func(right->context); 
}

bool or_matcher(void *context)
{
    logic_context_t *ctx = (logic_context_t *)context;
    linx_rule_match_t *left = (linx_rule_match_t *)ctx->left;
    linx_rule_match_t *right = (linx_rule_match_t *)ctx->right;
    bool result;

    result = left->func(left->context);
    if (result) {
        return result;
    }

    return result || right->func(right->context);
}

bool num_gt_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = linx_hash_map_get_value_ptr(ctx->base_addr, &ctx->field);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);
    return value > ctx->number.int_val;
}

bool num_ge_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = linx_hash_map_get_value_ptr(ctx->base_addr, &ctx->field);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);
    return value >= ctx->number.int_val;
}

bool num_lt_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = linx_hash_map_get_value_ptr(ctx->base_addr, &ctx->field);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);
    return value < ctx->number.int_val;
}

bool num_le_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = linx_hash_map_get_value_ptr(ctx->base_addr, &ctx->field);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);
    return value <= ctx->number.int_val;
}
