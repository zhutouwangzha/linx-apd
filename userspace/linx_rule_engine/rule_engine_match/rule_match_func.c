#include <string.h>
#include <ctype.h>

#include "rule_match_func.h"
#include "rule_match_struct.h"
#include "rule_match_context.h"
#include "output_match_func.h"

static char *linx_str_lower(char *str)
{
    char *lower, *p;
    
    if (!str) {
        return NULL;
    }

    lower =  malloc(strlen(str) + 1);
    if (!lower) {
        return NULL;
    }

    p = lower;

    while (*str) {
        *p++ = tolower((unsigned char)*str++);
    }
    
    *p = '\0';

    return lower;
}

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

bool not_matcher(void *context)
{
    unary_context_t *ctx = (unary_context_t *)context;
    linx_rule_match_t *op = (linx_rule_match_t *)ctx->operand;

    return !(op->func(op->context));
}

bool num_gt_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = linx_hash_map_get_value_ptr(&ctx->field);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);

    return value > ctx->number.int_val;
}

bool num_ge_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = linx_hash_map_get_value_ptr(&ctx->field);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);

    return value >= ctx->number.int_val;
}

bool num_lt_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = linx_hash_map_get_value_ptr(&ctx->field);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);

    return value < ctx->number.int_val;
}

bool num_le_matcher(void *context)
{
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = linx_hash_map_get_value_ptr(&ctx->field);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);

    return value <= ctx->number.int_val;
}

bool str_assign_matcher(void *context)
{
    int ret;
    str_context_t *ctx = (str_context_t *)context;
    char *value_ptr = linx_hash_map_get_value_ptr(&ctx->field);
    char *value;
    char buffer[256] = {0};

    switch (ctx->field.type) {
    case FIELD_TYPE_CHARBUF:
        value = value_ptr;
        break;
    case FILED_TYPE_CHARBUF_ARRAY:
        value = (char *)(*(uint64_t *)value_ptr);
        break;
    default:
        ret = format_field_value(&ctx->field, buffer, sizeof(buffer), 0);
        if (ret <= 0) {
            return false;
        }

        value = buffer;
        break;
    }

    if (strlen(value) != ctx->str_len) {
        return false;
    }

    ret = strncmp(value, ctx->str, ctx->str_len);

    return (ret == 0) ? true : false;
}

bool str_ne_matcher(void *context)
{
    return !str_assign_matcher(context);
}

bool str_contains_matcher(void *context)
{
    str_context_t *ctx = (str_context_t *)context;
    char *value_ptr = linx_hash_map_get_value_ptr(&ctx->field);
    char *value;
    const char *result;

    switch (ctx->field.type) {
    case FIELD_TYPE_CHARBUF:
        value = value_ptr;
        break;
    case FILED_TYPE_CHARBUF_ARRAY:
        value = (char *)(*(uint64_t *)value_ptr);
        break;
    default:
        return false;
        break;
    }

    result = strstr(value, ctx->str);

    return (result != NULL) ? true : false;
}

bool str_icontains_matcher(void *context)
{
    str_context_t *ctx = (str_context_t *)context;
    char *value_ptr = linx_hash_map_get_value_ptr(&ctx->field);
    char *value, *lower1, *lower2;
    const char *result;

    switch (ctx->field.type) {
    case FIELD_TYPE_CHARBUF:
        value = value_ptr;
        break;
    case FILED_TYPE_CHARBUF_ARRAY:
        value = (char *)(*(uint64_t *)value_ptr);
        break;
    default:
        return false;
        break;
    }

    lower1 = linx_str_lower(value);
    lower2 = linx_str_lower(ctx->str);

    if (lower1 && lower2) {
        result = strstr(lower1, lower2);
    } else {
        return false;
    }

    free(lower1);
    free(lower2);

    return (result != NULL) ? true : false;
}

bool str_startswith_matcher(void *context)
{
    str_context_t *ctx = (str_context_t *)context;
    char *value_ptr = linx_hash_map_get_value_ptr(&ctx->field);
    char *value;

    switch (ctx->field.type) {
    case FIELD_TYPE_CHARBUF:
        value = value_ptr;
        break;
    case FILED_TYPE_CHARBUF_ARRAY:
        value = (char *)(*(uint64_t *)value_ptr);
        break;
    default:
        return false;
        break;
    }

    if (ctx->str_len > strlen(value)) {
        return false;
    }

    return strncmp(value, ctx->str, ctx->str_len) == 0;
}

bool str_endswith_matcher(void *context)
{
    str_context_t *ctx = (str_context_t *)context;
    char *value_ptr = linx_hash_map_get_value_ptr(&ctx->field);
    char *value;
    size_t value_len;

    switch (ctx->field.type) {
    case FIELD_TYPE_CHARBUF:
        value = value_ptr;
        break;
    case FILED_TYPE_CHARBUF_ARRAY:
        value = (char *)(*(uint64_t *)value_ptr);
        break;
    default:
        return false;
        break;
    }

    value_len = strlen(value);

    if (ctx->str_len > value_len) {
        return false;
    }

    value = value + (value_len - ctx->str_len);

    return strncmp(value, ctx->str, ctx->str_len) == 0;
}

bool list_in_matcher(void *context)
{
    int ret;
    list_context_t *ctx = (list_context_t *)context;
    char *value_ptr = linx_hash_map_get_value_ptr(&ctx->field);
    char *value;
    size_t value_len;
    char buffer[256] = {0};

    switch (ctx->field.type) {
    case FIELD_TYPE_CHARBUF:
        value = value_ptr;
        break;
    case FILED_TYPE_CHARBUF_ARRAY:
        value = (char *)(*(uint64_t *)value_ptr);
        break;
    default:
        ret = format_field_value(&ctx->field, buffer, sizeof(buffer), 0);
        if (ret <= 0) {
            return false;
        }

        value = buffer;
        break;
    }

    value_len = strlen(value);

    for (size_t i = 0; i < ctx->list_count; ++i) {
        if (value_len != ctx->list_len[i]) {
            continue;
        }

        if (strncmp(value, ctx->list[i], ctx->list_len[i]) == 0) {
            return true;
        }
    }

    return false;
}
