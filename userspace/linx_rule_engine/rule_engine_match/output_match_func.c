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
    if (match == NULL) {
        return -1;
    }

    
}
