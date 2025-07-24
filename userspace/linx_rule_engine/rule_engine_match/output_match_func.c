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

static size_t format_field_value(segment_t *segment, char *buffer, size_t buffer_size, size_t total_length)
{
    size_t field_str_len = 0;
    char field_str[256] = {0};
    field_result_t *field = &segment->data.variable;
    void *value_ptr = linx_hash_map_get_value_ptr(field);

    if (!field->found || value_ptr == NULL) {
        return field_str_len;
    }

    switch (field->type) {
    case FIELD_TYPE_INT8:
        field_str_len = snprintf(field_str, sizeof(field_str), "%hhd", *(int8_t *)value_ptr);
        break;
    case FIELD_TYPE_UINT8:
        field_str_len = snprintf(field_str, sizeof(field_str), "%hhu", *(uint8_t *)value_ptr);
        break;
    case FIELD_TYPE_INT16:
        field_str_len = snprintf(field_str, sizeof(field_str), "%hd", *(int16_t *)value_ptr);
        break;
    case FIELD_TYPE_UINT16:
        field_str_len = snprintf(field_str, sizeof(field_str), "%hu", *(uint16_t *)value_ptr);
        break;
    case FIELD_TYPE_INT32:
        field_str_len = snprintf(field_str, sizeof(field_str), "%d", *(int32_t *)value_ptr);
        break;
    case FIELD_TYPE_UINT32:
        field_str_len = snprintf(field_str, sizeof(field_str), "%u", *(uint32_t *)value_ptr);
        break;
    case FIELD_TYPE_INT64:
        field_str_len = snprintf(field_str, sizeof(field_str), "%ld", *(int64_t *)value_ptr);
        break;
    case FIELD_TYPE_UINT64:
        field_str_len = snprintf(field_str, sizeof(field_str), "%lu", *(uint64_t *)value_ptr);
        break;
    case FIELD_TYPE_CHARBUF:
        field_str_len = snprintf(field_str, sizeof(field_str), "%s", (char *)value_ptr);
        break;
    case FILED_TYPE_CHARBUF_ARRAY:
        field_str_len = snprintf(field_str, sizeof(field_str), "%s", (char *)(*(uint64_t *)value_ptr));
        break;
    case FIELD_TYPE_BOOL:
        field_str_len = snprintf(field_str, sizeof(field_str), "%s", *(bool *)value_ptr ? "true" : "false");
        break;
    case FIELD_TYPE_FLOAT:
        field_str_len = snprintf(field_str, sizeof(field_str), "%f", *(float *)value_ptr);
        break;
    case FIELD_TYPE_DOUBLE:
        field_str_len = snprintf(field_str, sizeof(field_str), "%lf", *(double *)value_ptr);
        break;
    default:
        break;
    }

    if (field_str_len > 0 && 
        total_length + field_str_len < buffer_size - 1)
    {
        strncat(buffer + total_length, field_str, field_str_len);
    } else if (field_str_len > 0) {
        return -1;
    }

    return field_str_len;
}

int linx_output_match_format(linx_output_match_t *match, char *buffer, size_t buffer_size)
{
    size_t total_length = 0;
    size_t literal_len, field_len;
    segment_t *segment;

    if (match == NULL || buffer == NULL || buffer_size == 0) {
        return -1;
    }


    buffer[0] = '\0';

    for (size_t i = 0; i < match->size; i++) { 
        segment = match->segments[i];
        if (segment == NULL) {
            continue;
        }

        if (segment->type == SEGMENT_TYPE_LITERAL) {
            literal_len = segment->data.literal.length;
            if (total_length + literal_len >= buffer_size - 1) {
                return -1;
            }

            strncat(buffer + total_length, segment->data.literal.text, literal_len);
            total_length += literal_len;
        } else if (segment->type == SEGMENT_TYPE_VARIABLE) {
            field_len = format_field_value(segment, buffer, buffer_size, total_length);
            if (field_len == 0) {
                continue;
            }

            total_length += field_len;
        }
    }

    return total_length;
}

void linx_output_match_destroy(linx_output_match_t *match)
{
    segment_t *segment;

    if (!match) {
        return;
    }

    for (size_t i = 0; i < match->size; ++i) {
        segment = match->segments[i];

        if (segment &&
            segment->type == SEGMENT_TYPE_LITERAL) 
        {
            free(segment->data.literal.text);
            segment->data.literal.text = NULL;

            free(segment);
            match->segments[i] = NULL;
        }
    }

    free(match);
    match = NULL;
}
