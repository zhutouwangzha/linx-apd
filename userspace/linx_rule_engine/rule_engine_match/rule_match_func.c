#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "rule_match_func.h"
#include "rule_match_struct.h"
#include "rule_match_context.h"
#include "output_match_func.h"
#include "linx_field_type.h"
#include "linx_process_cache.h"
#include "linx_event_table.h"
#include "linx_name_value.h"
#include "field_struct.h"
#include "linx_hash_map_thread_safe.h"

static void *matcher_get_value_ptr(field_result_t *field, linx_field_type_t *type, size_t *size)
{
    void *ptr = linx_hash_map_get_value_ptr_thread_safe(field, type);

    if (ptr && field->type == LINX_FIELD_TYPE_STRUCT) {
        if (size) {
            *size = ((field_struct_t *)ptr)->size;
        }

        ptr = ((field_struct_t *)ptr)->data;
    } else if (size) {
        *size = field->size;
    }

    return ptr;
}

static int matcher_get_real_value_ptr(linx_field_type_t type, char **value, 
                                      char *value_ptr, char *buffer, size_t buf_size, 
                                      bool defalut_process, field_result_t *field)
{
    int ret = 0;
    uint32_t flags;
    size_t bytes_write = 0;
    linx_name_value_t *name_value;

    switch (type) {
    case LINX_FIELD_TYPE_CHARBUF:
    case LINX_FIELD_TYPE_UID:
    case LINX_FIELD_TYPE_PID:
    case LINX_FIELD_TYPE_BYTEBUF:
        *value = value_ptr;
        break;
    case LINX_FIELD_TYPE_CHARBUF_ARRAY:
    case LINX_FIELD_TYPE_CHARBUF_PAIR_ARRAY:
        *value = (char *)(*(uint64_t *)value_ptr);
        break;
    case LINX_FIELD_TYPE_FLAGS32:
        name_value = (linx_name_value_t *)(
            g_linx_event_table[*field->event_type].params[field->arg_index].info);
        if (name_value) {
            flags =  *(uint32_t *)value_ptr;

            for (int i = 0; name_value[i].name; ++i) {
                if (flags & name_value[i].value ||
                    flags == name_value[i].value) 
                {
                    bytes_write += snprintf(buffer + bytes_write, buf_size - bytes_write, 
                                            "%s", name_value[i].name);
                }
            }
        }

        *value = buffer;
        break;
    default:
        if (defalut_process) {
            ret = format_field_value(field, buffer, buf_size, 0);
            *value = buffer;
        }
        break;
    }

    if (*value == NULL) {
        ret = -1;
    }

    return ret;
}

static void *binary_search(const void *src, size_t src_len, const void *dst, size_t dst_len)
{
    const uint8_t *h, *n;
    size_t max_index = src_len - dst_len;

    if (dst_len == 0) {
        return (void *)src;
    }

    if (src == NULL || dst == NULL || src_len < dst_len) {
        return NULL;
    }

    h = src;
    n = dst;

    for (size_t i = 0; i < max_index; ++i) {
        if (h[i] != n[0]) {
            continue;
        }

        if (memcmp(h + i, n, dst_len) == 0) {
            return (void *)(h + i);
        }
    }

    return NULL;
}

/**
 * 该函数将字符串或二进制流中所有大写字母转换为小写
 * 不以 \0 终止操作，而是以传入的长度来判断
 * 处理完后依然会在最后新增一个 \0 结束符
 */
static char *linx_bin_lower(void *str, size_t size)
{
    char *lower, *p;

    if (!str) {
        return NULL;
    }

    lower = malloc(size + 1);
    if (!lower) {
        return NULL;
    }

    p = lower;

    for (size_t i = 0; i < size; ++i) {
        *p++ = tolower(*(uint8_t *)str++);
    }

    *p = '\0';

    return lower;
}

static char *linx_str_lower(void *str)
{
    if (!str) {
        return NULL;
    }

    return linx_bin_lower(str, strlen(str));
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
    linx_field_type_t type;
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = matcher_get_value_ptr(&ctx->field, &type, NULL);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);

    return value > ctx->number.int_val;
}

bool num_ge_matcher(void *context)
{
    linx_field_type_t type;
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = matcher_get_value_ptr(&ctx->field, &type, NULL);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);

    return value >= ctx->number.int_val;
}

bool num_lt_matcher(void *context)
{
    linx_field_type_t type;
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = matcher_get_value_ptr(&ctx->field, &type, NULL);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);

    return value < ctx->number.int_val;
}

bool num_le_matcher(void *context)
{
    linx_field_type_t type;
    num_context_t *ctx = (num_context_t *)context;
    void *value_ptr = matcher_get_value_ptr(&ctx->field, &type, NULL);
    if (!value_ptr) {
        return false;
    }
    long long value = (long long)(*(uint64_t *)value_ptr);

    return value <= ctx->number.int_val;
}

bool str_assign_matcher(void *context)
{
    int ret;
    char *value;
    char buffer[256] = {0};
    linx_field_type_t type;
    str_context_t *ctx = (str_context_t *)context;

    char *value_ptr = matcher_get_value_ptr(&ctx->field, &type, NULL);
    if (!value_ptr) {
        return false;
    }

    if (matcher_get_real_value_ptr(type, &value, value_ptr, 
                                   buffer, sizeof(buffer), true,
                                   &ctx->field) == -1) 
    {
        return false;
    }

    if (strlen(value) != ctx->str_len) {
        return false;
    }

    ret = memcmp(value, ctx->str, ctx->str_len);

    return (ret == 0) ? true : false;
}

bool str_ne_matcher(void *context)
{
    return !str_assign_matcher(context);
}

bool str_contains_matcher(void *context)
{
    char *value;
    size_t value_size;
    const char *result;
    char buffer[256] = {0};
    linx_field_type_t type;
    str_context_t *ctx = (str_context_t *)context;

    char *value_ptr = matcher_get_value_ptr(&ctx->field, &type, &value_size);
    if (!value_ptr) {
        return false;
    }

    if (matcher_get_real_value_ptr(type, &value, value_ptr, 
        buffer, sizeof(buffer), true,
        &ctx->field) == -1) 
    {
        return false;
    }

    if (type == LINX_FIELD_TYPE_BYTEBUF ||
        type == LINX_FIELD_TYPE_BYTEBUF_ARRAY)
    {
        result = binary_search(value, value_size, ctx->str, ctx->str_len);
    } else {
        result = strstr(value, ctx->str);
    }

    return (result != NULL) ? true : false;
}

bool str_icontains_matcher(void *context)
{
    char *value;
    size_t value_size;
    const char *result = NULL;
    char *lower1, *lower2;
    char buffer[256] = {0};
    linx_field_type_t type;
    str_context_t *ctx = (str_context_t *)context;

    char *value_ptr = matcher_get_value_ptr(&ctx->field, &type, &value_size);
    if (!value_ptr) {
        return false;
    }

    if (matcher_get_real_value_ptr(type, &value, value_ptr, 
        buffer, sizeof(buffer), true,
        &ctx->field) == -1) 
    {
        return false;
    }

    lower2 = linx_str_lower(ctx->str);
    if (!lower2) {
        goto out;
    }

    if (type == LINX_FIELD_TYPE_BYTEBUF ||
        type == LINX_FIELD_TYPE_BYTEBUF_ARRAY) 
    {
        lower1 = linx_bin_lower(value, value_size);
        result = binary_search(lower1, value_size, lower2, ctx->str_len);
    } else {
        lower1 = linx_str_lower(value);
        result = strstr(lower1, lower2);
    }

out:
    if (lower1) {
        free(lower1);
    }

    if (lower2) {
        free(lower2);
    }

    return (result != NULL) ? true : false;
}

bool str_startswith_matcher(void *context)
{
    char *value;
    char buffer[256] = {0};
    linx_field_type_t type;
    str_context_t *ctx = (str_context_t *)context;

    char *value_ptr = matcher_get_value_ptr(&ctx->field, &type, NULL);
    if (!value_ptr) {
        return false;
    }

    if (matcher_get_real_value_ptr(type, &value, value_ptr, 
        buffer, sizeof(buffer), true,
        &ctx->field) == -1) 
    {
        return false;
    }

    if (ctx->str_len > strlen(value)) {
        return false;
    }

    return strncmp(value, ctx->str, ctx->str_len) == 0;
}

bool str_endswith_matcher(void *context)
{
    char *value;
    size_t value_len;
    char buffer[256] = {0};
    linx_field_type_t type;
    str_context_t *ctx = (str_context_t *)context;

    char *value_ptr = matcher_get_value_ptr(&ctx->field, &type, NULL);
    if (!value_ptr) {
        return false;
    }

    if (matcher_get_real_value_ptr(type, &value, value_ptr, 
        buffer, sizeof(buffer), true,
        &ctx->field) == -1) 
    {
        return false;
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
    char *value;
    size_t value_len;
    char buffer[256] = {0};
    linx_field_type_t type;
    list_context_t *ctx = (list_context_t *)context;

    char *value_ptr = matcher_get_value_ptr(&ctx->field, &type, NULL);
    if (!value_ptr) {
        return false;
    }

    if (matcher_get_real_value_ptr(type, &value, value_ptr, 
        buffer, sizeof(buffer), true,
        &ctx->field) == -1) 
    {
        return false;
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

bool val_matcher(void *context)
{
    str_context_t *str_ctx;
    linx_field_type_t type;
    bool result, need_free = false;
    char *value, *value_ptr, *buffer;
    val_context_t *ctx = (val_context_t *)context;
    linx_rule_match_t *op = (linx_rule_match_t *)ctx->operand;

    value_ptr = matcher_get_value_ptr(&ctx->field, &type, NULL);
    if (!value_ptr) {
        return false;
    }

    switch (op->type) {
    case MATCH_CONTEXT_NUM:
        num_context_t *num_ctx = (num_context_t *)op->context;
        if (type == LINX_FIELD_TYPE_DOUBLE) {
            num_ctx->number.double_val = (double)(*(uint64_t *)value_ptr);
        } else {
            num_ctx->number.int_val = (long long)(*(uint64_t *)value_ptr);
        }
        break;
    case MATCH_CONTEXT_STR:
        str_ctx = (str_context_t *)op->context;

        buffer = malloc(256);
        if (!buffer) {
            return false;
        }

        if (matcher_get_real_value_ptr(type, &value, value_ptr,
                                       buffer, 256, true,
                                       &ctx->field) == -1)
        {
            free(buffer);
            return false;
        }

        need_free = true;
        str_ctx->str = value;
        str_ctx->str_len = strlen(value);
        break;
    default:
        break;
    }

    result =  op->func(op->context);

    if (need_free) {
        free(buffer);
        str_ctx->str = NULL;
    }

    return result;
}
