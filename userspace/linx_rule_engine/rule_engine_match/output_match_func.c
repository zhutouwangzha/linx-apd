#include <string.h>
#include <stdio.h>
#include "output_match_func.h"
#include "linx_hash_map.h"

static int resize_segments(linx_output_match_t *match)
{
    size_t new_capacity;
    segment_t **new_segments;

    if (match == NULL) {
        return -1;
    }

    new_capacity = match->capacity ? match->capacity * 2 : 2;

    new_segments = realloc(match->segments, new_capacity * sizeof(segment_t *));
    if (new_segments == NULL) {
        return -1;
    }

    match->capacity = new_capacity;
    match->segments = new_segments;

    return 0;
}

static int add_literal_segment(linx_output_match_t *match, char *literal, size_t length)
{
    segment_t *segment;

    if (match == NULL || literal == NULL || length == 0) {
        return -1;
    }

    if (match->size >= match->capacity) {
        if (resize_segments(match)) {
            return -1;
        }
    }

    segment = malloc(sizeof(segment_t));
    if (segment == NULL) {
        return -1;
    }
 
    segment->type = SEGMENT_TYPE_LITERAL;
    segment->data.literal.text = strndup(literal, length);
    segment->data.literal.length = length;

    if (!segment->data.literal.text) {
        free(segment);
        return -1;
    }

    match->segments[match->size++] = segment;

    return 0;
}

static int add_variable_segment(linx_output_match_t *match, char *variable)
{
    segment_t *segment;

    if (match == NULL || variable == NULL) {
        return -1;
    }

    if (match->size >= match->capacity) {
        if (resize_segments(match)) {
            return -1;
        }
    }

    segment = malloc(sizeof(segment_t));
    if (segment == NULL) {
        return -1;
    }

    segment->type = SEGMENT_TYPE_VARIABLE;
    segment->data.variable = linx_hash_map_get_field_by_path(variable);

    if (segment->data.variable.found == false) {
        free(segment);
        return -1;
    }

    match->segments[match->size++] = segment;

    return 0;
}

int linx_output_match_compile(linx_output_match_t **match, char *format)
{
    char *current = format, *start = format;
    char *var_start, *variable_name;
    size_t var_len;

    if (format == NULL) {
        return -1;
    }

    *match = malloc(sizeof(linx_output_match_t));
    if (*match == NULL) {
        return -1;
    }

    (*match)->segments = NULL;
    (*match)->size = 0;
    (*match)->capacity = 0;

    while (*current) {
        if (*current == '%') {
            if (current > start) {
                if (add_literal_segment(*match, start, current - start)) {
                    return -1;
                }
            }
        
            current++;
            var_start = current;

            while (*current && *current != ' ' &&
                   *current != '\t' && *current != '\n' && 
                   *current != '\r' && *current != ')' &&
                   *current != '(')
            {
                current++;
            }
            
            var_len = current - var_start;
            variable_name = strndup(var_start, var_len);
            if (!variable_name) {
                return -1;
            }

            if (add_variable_segment(*match, variable_name)) {
                return -1;
            }

            free(variable_name);
            start = current;
        } else {
            current++;
        }
    }

    if (current > start) {
        if (add_literal_segment(*match, start, current - start)) {
            return -1;
        }
    }

    return 0;
}

int linx_output_match_format(linx_output_match_t *match, char *buffer, size_t buffer_size)
{
    if (match == NULL || buffer == NULL || buffer_size == 0) {
        return -1;
    }

    size_t total_length = 0;
    buffer[0] = '\0';  // 初始化为空字符串

    for (size_t i = 0; i < match->size; i++) {
        segment_t *segment = match->segments[i];
        if (segment == NULL) {
            continue;
        }

        if (segment->type == SEGMENT_TYPE_LITERAL) {
            // 处理字面量段
            size_t literal_len = segment->data.literal.length;
            if (total_length + literal_len >= buffer_size - 1) {
                return -1;  // 缓冲区空间不足
            }
            
            strncat(buffer, segment->data.literal.text, literal_len);
            total_length += literal_len;
            
        } else if (segment->type == SEGMENT_TYPE_VARIABLE) {
            // 处理变量段
            field_result_t *field = &segment->data.variable;
            if (!field->found || field->value_ptr == NULL) {
                continue;  // 字段未找到或值为空，跳过
            }

            char field_str[256] = {0};
            int field_str_len = 0;

            // 根据字段类型格式化值
            switch (field->type) {
                case FIELD_TYPE_INT:
                    field_str_len = snprintf(field_str, sizeof(field_str), "%d", *(int*)field->value_ptr);
                    break;
                case FIELD_TYPE_LONG:
                    field_str_len = snprintf(field_str, sizeof(field_str), "%ld", *(long*)field->value_ptr);
                    break;
                case FIELD_TYPE_UINT:
                    field_str_len = snprintf(field_str, sizeof(field_str), "%u", *(unsigned int*)field->value_ptr);
                    break;
                case FIELD_TYPE_ULONG:
                    field_str_len = snprintf(field_str, sizeof(field_str), "%lu", *(unsigned long*)field->value_ptr);
                    break;
                case FIELD_TYPE_STRING:
                    field_str_len = snprintf(field_str, sizeof(field_str), "%s", (char*)field->value_ptr);
                    break;
                case FIELD_TYPE_CHAR:
                    field_str_len = snprintf(field_str, sizeof(field_str), "%c", *(char*)field->value_ptr);
                    break;
                default:
                    continue;  // 未知类型，跳过
            }

            if (field_str_len > 0 && total_length + field_str_len < buffer_size - 1) {
                strcat(buffer, field_str);
                total_length += field_str_len;
            } else if (field_str_len > 0) {
                return -1;  // 缓冲区空间不足
            }
        }
    }

    return total_length;
}
