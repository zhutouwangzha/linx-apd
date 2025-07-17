#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stddef.h>

#include "linx_output_match.h"
#include "linx_event_data.h"

// 默认容量
#define DEFAULT_SEGMENT_CAPACITY 16
#define DEFAULT_BINDING_CAPACITY 8
#define MAX_FIELD_VALUE_SIZE 256

linx_output_match_t *linx_output_match_create(void) {
    linx_output_match_t *output_match = calloc(1, sizeof(linx_output_match_t));
    if (!output_match) {
        return NULL;
    }
    
    // 初始化片段数组
    output_match->segments = calloc(DEFAULT_SEGMENT_CAPACITY, sizeof(output_segment_t));
    if (!output_match->segments) {
        free(output_match);
        return NULL;
    }
    output_match->segment_capacity = DEFAULT_SEGMENT_CAPACITY;
    
    // 初始化绑定数组
    output_match->bindings = calloc(DEFAULT_BINDING_CAPACITY, sizeof(field_binding_t));
    if (!output_match->bindings) {
        free(output_match->segments);
        free(output_match);
        return NULL;
    }
    output_match->binding_capacity = DEFAULT_BINDING_CAPACITY;
    
    return output_match;
}

void linx_output_match_destroy(linx_output_match_t *output_match) {
    if (!output_match) {
        return;
    }
    
    // 释放片段数据
    for (size_t i = 0; i < output_match->segment_count; i++) {
        if (output_match->segments[i].type == SEGMENT_TYPE_LITERAL) {
            free(output_match->segments[i].data.literal.text);
        }
    }
    free(output_match->segments);
    
    // 释放绑定数据
    for (size_t i = 0; i < output_match->binding_count; i++) {
        free((void*)output_match->bindings[i].variable_name);
    }
    free(output_match->bindings);
    
    free(output_match->original_template);
    free(output_match);
}

static int resize_segments(linx_output_match_t *output_match) {
    size_t new_capacity = output_match->segment_capacity * 2;
    output_segment_t *new_segments = realloc(output_match->segments, 
                                            new_capacity * sizeof(output_segment_t));
    if (!new_segments) {
        return -1;
    }
    
    output_match->segments = new_segments;
    output_match->segment_capacity = new_capacity;
    return 0;
}

static int resize_bindings(linx_output_match_t *output_match) {
    size_t new_capacity = output_match->binding_capacity * 2;
    field_binding_t *new_bindings = realloc(output_match->bindings,
                                           new_capacity * sizeof(field_binding_t));
    if (!new_bindings) {
        return -1;
    }
    
    output_match->bindings = new_bindings;
    output_match->binding_capacity = new_capacity;
    return 0;
}

int linx_output_match_add_binding(linx_output_match_t *output_match,
                                  const char *variable_name,
                                  field_type_t field_type,
                                  size_t offset,
                                  size_t size) {
    if (!output_match || !variable_name) {
        return -1;
    }
    
    // 检查是否需要扩容
    if (output_match->binding_count >= output_match->binding_capacity) {
        if (resize_bindings(output_match) != 0) {
            return -1;
        }
    }
    
    // 添加绑定
    field_binding_t *binding = &output_match->bindings[output_match->binding_count];
    binding->variable_name = strdup(variable_name);
    binding->field_type = field_type;
    binding->offset = offset;
    binding->size = size;
    binding->custom_handler = NULL;
    
    return output_match->binding_count++;
}

int linx_output_match_add_custom_binding(linx_output_match_t *output_match,
                                         const char *variable_name,
                                         int (*custom_handler)(const void *data, char *output, size_t output_size)) {
    if (!output_match || !variable_name || !custom_handler) {
        return -1;
    }
    
    // 检查是否需要扩容
    if (output_match->binding_count >= output_match->binding_capacity) {
        if (resize_bindings(output_match) != 0) {
            return -1;
        }
    }
    
    // 添加自定义绑定
    field_binding_t *binding = &output_match->bindings[output_match->binding_count];
    binding->variable_name = strdup(variable_name);
    binding->field_type = FIELD_TYPE_CUSTOM;
    binding->offset = 0;
    binding->size = 0;
    binding->custom_handler = custom_handler;
    
    return output_match->binding_count++;
}

static int find_binding_by_name(const linx_output_match_t *output_match, const char *variable_name) {
    for (size_t i = 0; i < output_match->binding_count; i++) {
        if (strcmp(output_match->bindings[i].variable_name, variable_name) == 0) {
            return i;
        }
    }
    return -1;
}

static int add_literal_segment(linx_output_match_t *output_match, const char *text, size_t length) {
    // 检查是否需要扩容
    if (output_match->segment_count >= output_match->segment_capacity) {
        if (resize_segments(output_match) != 0) {
            return -1;
        }
    }
    
    output_segment_t *segment = &output_match->segments[output_match->segment_count];
    segment->type = SEGMENT_TYPE_LITERAL;
    segment->data.literal.text = strndup(text, length);
    segment->data.literal.length = length;
    
    if (!segment->data.literal.text) {
        return -1;
    }
    
    output_match->segment_count++;
    output_match->estimated_output_size += length;
    return 0;
}

static int add_variable_segment(linx_output_match_t *output_match, const char *variable_name) {
    // 查找对应的绑定
    int binding_index = find_binding_by_name(output_match, variable_name);
    if (binding_index == -1) {
        // 绑定不存在，可能需要在编译时报错或者跳过
        return -1;
    }
    
    // 检查是否需要扩容
    if (output_match->segment_count >= output_match->segment_capacity) {
        if (resize_segments(output_match) != 0) {
            return -1;
        }
    }
    
    output_segment_t *segment = &output_match->segments[output_match->segment_count];
    segment->type = SEGMENT_TYPE_VARIABLE;
    segment->data.variable.binding = &output_match->bindings[binding_index];
    segment->data.variable.binding_index = binding_index;
    
    output_match->segment_count++;
    output_match->estimated_output_size += MAX_FIELD_VALUE_SIZE; // 预估字段值大小
    return 0;
}

int linx_output_match_compile(linx_output_match_t *output_match, const char *template_str) {
    if (!output_match || !template_str) {
        return -1;
    }
    
    // 保存原始模板
    output_match->original_template = strdup(template_str);
    if (!output_match->original_template) {
        return -1;
    }
    
    const char *current = template_str;
    const char *start = current;
    
    while (*current) {
        if (*current == '%') {
            // 找到变量开始
            // 先添加之前的字面量文本
            if (current > start) {
                if (add_literal_segment(output_match, start, current - start) != 0) {
                    return -1;
                }
            }
            
            // 查找变量结束位置
            const char *var_start = current;
            current++; // 跳过 '%'
            
            // 查找变量名结束位置（空格、换行或字符串结束）
            while (*current && *current != ' ' && *current != '\t' && 
                   *current != '\n' && *current != '\r') {
                current++;
            }
            
            // 提取变量名
            size_t var_len = current - var_start;
            char *variable_name = strndup(var_start, var_len);
            if (!variable_name) {
                return -1;
            }
            
            // 添加变量片段
            if (add_variable_segment(output_match, variable_name) != 0) {
                free(variable_name);
                return -1;
            }
            
            free(variable_name);
            start = current;
        } else {
            current++;
        }
    }
    
    // 添加最后的字面量文本
    if (current > start) {
        if (add_literal_segment(output_match, start, current - start) != 0) {
            return -1;
        }
    }
    
    return 0;
}

static int format_field_value(const field_binding_t *binding, const void *data_struct, 
                             char *output, size_t output_size) {
    if (!binding || !data_struct || !output) {
        return -1;
    }
    
    const char *data_ptr = (const char*)data_struct + binding->offset;
    
    switch (binding->field_type) {
        case FIELD_TYPE_STRING: {
            const char *str_val = *(const char**)data_ptr;
            if (str_val) {
                snprintf(output, output_size, "%s", str_val);
            } else {
                snprintf(output, output_size, "null");
            }
            break;
        }
        
        case FIELD_TYPE_INT32: {
            int32_t int_val = *(const int32_t*)data_ptr;
            snprintf(output, output_size, "%d", int_val);
            break;
        }
        
        case FIELD_TYPE_INT64: {
            int64_t int_val = *(const int64_t*)data_ptr;
            snprintf(output, output_size, "%ld", int_val);
            break;
        }
        
        case FIELD_TYPE_UINT32: {
            uint32_t uint_val = *(const uint32_t*)data_ptr;
            snprintf(output, output_size, "%u", uint_val);
            break;
        }
        
        case FIELD_TYPE_UINT64: {
            uint64_t uint_val = *(const uint64_t*)data_ptr;
            snprintf(output, output_size, "%lu", uint_val);
            break;
        }
        
        case FIELD_TYPE_UINT16: {
            uint16_t uint_val = *(const uint16_t*)data_ptr;
            snprintf(output, output_size, "%u", uint_val);
            break;
        }
        
        case FIELD_TYPE_TIMESTAMP: {
            const struct linx_timespec *ts = (const struct linx_timespec*)data_ptr;
            struct tm *tm_info = localtime(&ts->tv_sec);
            snprintf(output, output_size, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
                     tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                     tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                     ts->tv_nsec / 1000000);
            break;
        }
        
        case FIELD_TYPE_CUSTOM: {
            if (binding->custom_handler) {
                return binding->custom_handler(data_struct, output, output_size);
            }
            snprintf(output, output_size, "custom_error");
            break;
        }
        
        default:
            snprintf(output, output_size, "unknown_type");
            break;
    }
    
    return 0;
}

int linx_output_match_format(const linx_output_match_t *output_match, 
                            const void *data_struct,
                            char *output_buffer, 
                            size_t buffer_size) {
    if (!output_match || !data_struct || !output_buffer) {
        return -1;
    }
    
    char *current = output_buffer;
    size_t remaining = buffer_size;
    
    for (size_t i = 0; i < output_match->segment_count; i++) {
        const output_segment_t *segment = &output_match->segments[i];
        
        if (segment->type == SEGMENT_TYPE_LITERAL) {
            // 直接复制字面量文本
            size_t copy_len = segment->data.literal.length;
            if (copy_len >= remaining) {
                copy_len = remaining - 1;
            }
            
            memcpy(current, segment->data.literal.text, copy_len);
            current += copy_len;
            remaining -= copy_len;
            
        } else if (segment->type == SEGMENT_TYPE_VARIABLE) {
            // 格式化变量值
            char field_value[MAX_FIELD_VALUE_SIZE];
            if (format_field_value(segment->data.variable.binding, data_struct, 
                                  field_value, sizeof(field_value)) == 0) {
                size_t value_len = strlen(field_value);
                if (value_len >= remaining) {
                    value_len = remaining - 1;
                }
                
                memcpy(current, field_value, value_len);
                current += value_len;
                remaining -= value_len;
            }
        }
        
        if (remaining <= 1) {
            break; // 缓冲区不够
        }
    }
    
    *current = '\0';
    return 0;
}

int linx_output_match_format_and_print(const linx_output_match_t *output_match,
                                       const void *data_struct) {
    if (!output_match || !data_struct) {
        return -1;
    }
    
    // 分配输出缓冲区
    size_t buffer_size = output_match->estimated_output_size + 1;
    char *output_buffer = malloc(buffer_size);
    if (!output_buffer) {
        return -1;
    }
    
    // 格式化输出
    int ret = linx_output_match_format(output_match, data_struct, output_buffer, buffer_size);
    if (ret == 0) {
        printf("%s\n", output_buffer);
        fflush(stdout);
    }
    
    free(output_buffer);
    return ret;
}