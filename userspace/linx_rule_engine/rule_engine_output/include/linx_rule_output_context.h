#ifndef __LINX_RULE_OUTPUT_CONTEXT_H__
#define __LINX_RULE_OUTPUT_CONTEXT_H__

#include <stdint.h>
#include <time.h>

/**
 * @brief 事件上下文结构体
 * 
 * 用于传递事件相关的数据给输出格式化函数
 */
typedef struct {
    // 时间相关
    struct timespec timestamp;
    
    // 进程相关
    pid_t pid;
    pid_t ppid;
    uid_t uid;
    char *process_name;
    char *cmdline;
    
    // 文件相关
    char *file_name;
    char *file_path;
    
    // 事件相关
    uint32_t event_type;
    char *event_direction;
    char *event_data;
    
    // 网络相关
    char *src_ip;
    char *dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    char *protocol;
    
    // 用户相关
    char *username;
    
} linx_rule_output_context_t;

/**
 * @brief 创建输出上下文
 * 
 * @return linx_rule_output_context_t* 新创建的上下文指针，失败返回NULL
 */
linx_rule_output_context_t *linx_rule_output_context_create(void);

/**
 * @brief 销毁输出上下文
 * 
 * @param context 要销毁的上下文指针
 */
void linx_rule_output_context_destroy(linx_rule_output_context_t *context);

/**
 * @brief 使用事件上下文格式化输出字符串
 * 
 * @param output_template 输出模板字符串
 * @param context 事件上下文
 * @param formatted_output 格式化后的输出字符串（需要调用者释放）
 * @return int 0表示成功，-1表示失败
 */
int linx_rule_output_format_string_with_context(const char *output_template, 
                                                const linx_rule_output_context_t *context,
                                                char **formatted_output);

/**
 * @brief 使用事件上下文格式化并输出规则匹配结果
 * 
 * @param rule 匹配的规则
 * @param context 事件上下文
 * @return int 0表示成功，-1表示失败
 */
int linx_rule_output_format_and_print_with_context(const linx_rule_t *rule,
                                                   const linx_rule_output_context_t *context);

#endif /* __LINX_RULE_OUTPUT_CONTEXT_H__ */