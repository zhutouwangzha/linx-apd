#include <string.h>

#include "rule_match_func.h"
#include "rule_match_struct.h"
#include "rule_match_context.h"

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
    long long value = (long long)(*(uint64_t *)ctx->field.value_ptr);
    return value > ctx->number.int_val;
}

bool num_ge_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    long long value = (long long)(*(uint64_t *)ctx->field.value_ptr);
    return value >= ctx->number.int_val;
}

bool num_lt_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    long long value = (long long)(*(uint64_t *)ctx->field.value_ptr);
    return value < ctx->number.int_val;
}

bool num_le_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    long long value = (long long)(*(uint64_t *)ctx->field.value_ptr);
    return value <= ctx->number.int_val;
}
