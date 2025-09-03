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

/**
 * 线程安全版本的获取值指针函数
 */
static void *matcher_get_value_ptr_thread_safe(field_result_t *field, linx_field_type_t *type, size_t *size)
{
    void *ptr = linx_hash_map_thread_local_get_value_ptr(field, type);

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

/**
 * 线程安全版本的字符串匹配函数
 */
bool rule_match_string_thread_safe(rule_match_context_t *context)
{
    field_result_t field;
    linx_field_type_t type;
    char *value, *value_ptr, *target;
    size_t size;
    char buffer[1024] = {0};
    bool ret = false;

    if (!context || !context->rule_match || !context->rule_match->rule_string) {
        return false;
    }

    /* 使用线程本地的字段查询 */
    field = linx_hash_map_thread_local_get_field_by_path(strdup(context->rule_match->rule_string->field_path));
    if (!field.found) {
        goto cleanup;
    }

    value_ptr = matcher_get_value_ptr_thread_safe(&field, &type, &size);
    if (!value_ptr) {
        goto cleanup;
    }

    if (matcher_get_real_value_ptr(type, &value, value_ptr, buffer, sizeof(buffer), true, &field) != 0) {
        goto cleanup;
    }

    target = context->rule_match->rule_string->value;

    switch (context->rule_match->rule_string->operator) {
    case RULE_MATCH_OPERATOR_EQUAL:
        ret = (strcmp(value, target) == 0);
        break;
    case RULE_MATCH_OPERATOR_NOT_EQUAL:
        ret = (strcmp(value, target) != 0);
        break;
    case RULE_MATCH_OPERATOR_CONTAINS:
        ret = (strstr(value, target) != NULL);
        break;
    case RULE_MATCH_OPERATOR_NOT_CONTAINS:
        ret = (strstr(value, target) == NULL);
        break;
    case RULE_MATCH_OPERATOR_STARTS_WITH:
        ret = (strncmp(value, target, strlen(target)) == 0);
        break;
    case RULE_MATCH_OPERATOR_ENDS_WITH:
        {
            size_t value_len = strlen(value);
            size_t target_len = strlen(target);
            if (value_len >= target_len) {
                ret = (strcmp(value + value_len - target_len, target) == 0);
            }
        }
        break;
    default:
        ret = false;
        break;
    }

cleanup:
    if (field.arg) {
        free(field.arg);
    }
    return ret;
}

/**
 * 线程安全版本的数字匹配函数
 */
bool rule_match_number_thread_safe(rule_match_context_t *context)
{
    field_result_t field;
    linx_field_type_t type;
    void *value_ptr;
    int64_t value = 0, target;
    bool ret = false;

    if (!context || !context->rule_match || !context->rule_match->rule_number) {
        return false;
    }

    /* 使用线程本地的字段查询 */
    field = linx_hash_map_thread_local_get_field_by_path(strdup(context->rule_match->rule_number->field_path));
    if (!field.found) {
        goto cleanup;
    }

    value_ptr = matcher_get_value_ptr_thread_safe(&field, &type, NULL);
    if (!value_ptr) {
        goto cleanup;
    }

    /* 根据类型提取数值 */
    switch (type) {
    case LINX_FIELD_TYPE_INT8:
        value = *(int8_t *)value_ptr;
        break;
    case LINX_FIELD_TYPE_INT16:
        value = *(int16_t *)value_ptr;
        break;
    case LINX_FIELD_TYPE_INT32:
        value = *(int32_t *)value_ptr;
        break;
    case LINX_FIELD_TYPE_INT64:
        value = *(int64_t *)value_ptr;
        break;
    case LINX_FIELD_TYPE_UINT8:
        value = *(uint8_t *)value_ptr;
        break;
    case LINX_FIELD_TYPE_UINT16:
        value = *(uint16_t *)value_ptr;
        break;
    case LINX_FIELD_TYPE_UINT32:
        value = *(uint32_t *)value_ptr;
        break;
    case LINX_FIELD_TYPE_UINT64:
        value = *(uint64_t *)value_ptr;
        break;
    default:
        goto cleanup;
    }

    target = context->rule_match->rule_number->value;

    switch (context->rule_match->rule_number->operator) {
    case RULE_MATCH_OPERATOR_EQUAL:
        ret = (value == target);
        break;
    case RULE_MATCH_OPERATOR_NOT_EQUAL:
        ret = (value != target);
        break;
    case RULE_MATCH_OPERATOR_GREATER_THAN:
        ret = (value > target);
        break;
    case RULE_MATCH_OPERATOR_GREATER_EQUAL:
        ret = (value >= target);
        break;
    case RULE_MATCH_OPERATOR_LESS_THAN:
        ret = (value < target);
        break;
    case RULE_MATCH_OPERATOR_LESS_EQUAL:
        ret = (value <= target);
        break;
    default:
        ret = false;
        break;
    }

cleanup:
    if (field.arg) {
        free(field.arg);
    }
    return ret;
}

/**
 * 线程安全版本的主匹配函数
 */
bool rule_match_func_thread_safe(rule_match_context_t *context)
{
    bool result = false;

    if (!context || !context->rule_match) {
        return false;
    }

    switch (context->rule_match->type) {
    case RULE_MATCH_TYPE_STRING:
        result = rule_match_string_thread_safe(context);
        break;
    case RULE_MATCH_TYPE_NUMBER:
        result = rule_match_number_thread_safe(context);
        break;
    case RULE_MATCH_TYPE_AND:
        if (context->rule_match->rule_and && 
            context->rule_match->rule_and->left && 
            context->rule_match->rule_and->right) {
            
            rule_match_context_t left_context = *context;
            rule_match_context_t right_context = *context;
            
            left_context.rule_match = context->rule_match->rule_and->left;
            right_context.rule_match = context->rule_match->rule_and->right;
            
            result = rule_match_func_thread_safe(&left_context) && 
                    rule_match_func_thread_safe(&right_context);
        }
        break;
    case RULE_MATCH_TYPE_OR:
        if (context->rule_match->rule_or && 
            context->rule_match->rule_or->left && 
            context->rule_match->rule_or->right) {
            
            rule_match_context_t left_context = *context;
            rule_match_context_t right_context = *context;
            
            left_context.rule_match = context->rule_match->rule_or->left;
            right_context.rule_match = context->rule_match->rule_or->right;
            
            result = rule_match_func_thread_safe(&left_context) || 
                    rule_match_func_thread_safe(&right_context);
        }
        break;
    case RULE_MATCH_TYPE_NOT:
        if (context->rule_match->rule_not && 
            context->rule_match->rule_not->operand) {
            
            rule_match_context_t not_context = *context;
            not_context.rule_match = context->rule_match->rule_not->operand;
            
            result = !rule_match_func_thread_safe(&not_context);
        }
        break;
    default:
        result = false;
        break;
    }

    return result;
}