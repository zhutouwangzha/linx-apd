#ifndef __OUTPUT_MATCH_STRUCT_H__
#define __OUTPUT_MATCH_STRUCT_H__ 

#include "linx_hash_map.h"

typedef enum {
    SEGMENT_TYPE_LITERAL,   /* 字面量文本，可直接输出 */
    SEGMENT_TYPE_VARIABLE,  /* 变量，需要从字段表中获取 */
    SEGMENT_TYPE_MAX
} segment_typet_t;

typedef struct {
    segment_typet_t type;
    union {
        struct {
            char *text;
            size_t length;
        } literal;

        field_result_t variable;
    } data;
} segment_t;

typedef struct {
    segment_t **segments;
    size_t size;
    size_t capacity;
} linx_output_match_t;

#endif /* __OUTPUT_MATCH_STRUCT_H__ */
