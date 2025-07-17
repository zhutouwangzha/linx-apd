#ifndef __LINX_OUTPUT_MATCH_H__
#define __LINX_OUTPUT_MATCH_H__

#include <stddef.h>
#include <stdint.h>

// 前向声明
typedef struct linx_output_match linx_output_match_t;

/**
 * @brief 字段类型枚举
 */
typedef enum {
    FIELD_TYPE_STRING,      // 字符串类型
    FIELD_TYPE_INT32,       // 32位整数
    FIELD_TYPE_INT64,       // 64位整数
    FIELD_TYPE_UINT32,      // 32位无符号整数
    FIELD_TYPE_UINT64,      // 64位无符号整数
    FIELD_TYPE_UINT16,      // 16位无符号整数
    FIELD_TYPE_TIMESTAMP,   // 时间戳类型
    FIELD_TYPE_CUSTOM,      // 自定义处理函数
    FIELD_TYPE_MAX
} field_type_t;

/**
 * @brief 字段绑定信息结构体
 */
typedef struct {
    const char *variable_name;      // 变量名，如"%evt.time"
    field_type_t field_type;        // 字段类型
    size_t offset;                  // 在数据结构中的偏移量
    size_t size;                    // 字段大小
    
    // 自定义处理函数（当field_type为FIELD_TYPE_CUSTOM时使用）
    int (*custom_handler)(const void *data, char *output, size_t output_size);
} field_binding_t;

/**
 * @brief 输出片段类型
 */
typedef enum {
    SEGMENT_TYPE_LITERAL,   // 字面量文本
    SEGMENT_TYPE_VARIABLE   // 变量引用
} segment_type_t;

/**
 * @brief 输出片段结构体
 */
typedef struct {
    segment_type_t type;
    union {
        struct {
            char *text;             // 字面量文本
            size_t length;          // 文本长度
        } literal;
        
        struct {
            const field_binding_t *binding;  // 字段绑定信息
            size_t binding_index;           // 绑定索引
        } variable;
    } data;
} output_segment_t;

/**
 * @brief 输出匹配结构体
 * 
 * 预编译的输出模板，包含解析后的片段和字段绑定信息
 */
struct linx_output_match {
    // 输出片段数组
    output_segment_t *segments;
    size_t segment_count;
    size_t segment_capacity;
    
    // 字段绑定数组
    field_binding_t *bindings;
    size_t binding_count;
    size_t binding_capacity;
    
    // 预估的最大输出长度
    size_t estimated_output_size;
    
    // 原始模板字符串（用于调试）
    char *original_template;
};

/**
 * @brief 创建输出匹配结构体
 * 
 * @return linx_output_match_t* 新创建的结构体指针，失败返回NULL
 */
linx_output_match_t *linx_output_match_create(void);

/**
 * @brief 销毁输出匹配结构体
 * 
 * @param output_match 要销毁的结构体指针
 */
void linx_output_match_destroy(linx_output_match_t *output_match);

/**
 * @brief 编译输出模板
 * 
 * 将输出模板字符串解析为预编译的片段和字段绑定
 * 
 * @param output_match 输出匹配结构体
 * @param template_str 模板字符串
 * @return int 0表示成功，-1表示失败
 */
int linx_output_match_compile(linx_output_match_t *output_match, const char *template_str);

/**
 * @brief 使用预编译的输出匹配快速生成输出
 * 
 * @param output_match 预编译的输出匹配结构体
 * @param data_struct 数据结构体指针
 * @param output_buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 0表示成功，-1表示失败
 */
int linx_output_match_format(const linx_output_match_t *output_match, 
                            const void *data_struct,
                            char *output_buffer, 
                            size_t buffer_size);

/**
 * @brief 使用预编译的输出匹配快速生成并打印输出
 * 
 * @param output_match 预编译的输出匹配结构体
 * @param data_struct 数据结构体指针
 * @return int 0表示成功，-1表示失败
 */
int linx_output_match_format_and_print(const linx_output_match_t *output_match,
                                       const void *data_struct);

/**
 * @brief 添加字段绑定
 * 
 * @param output_match 输出匹配结构体
 * @param variable_name 变量名
 * @param field_type 字段类型
 * @param offset 偏移量
 * @param size 字段大小
 * @return int 绑定索引，失败返回-1
 */
int linx_output_match_add_binding(linx_output_match_t *output_match,
                                  const char *variable_name,
                                  field_type_t field_type,
                                  size_t offset,
                                  size_t size);

/**
 * @brief 添加自定义字段绑定
 * 
 * @param output_match 输出匹配结构体
 * @param variable_name 变量名
 * @param custom_handler 自定义处理函数
 * @return int 绑定索引，失败返回-1
 */
int linx_output_match_add_custom_binding(linx_output_match_t *output_match,
                                         const char *variable_name,
                                         int (*custom_handler)(const void *data, char *output, size_t output_size));

#endif /* __LINX_OUTPUT_MATCH_H__ */