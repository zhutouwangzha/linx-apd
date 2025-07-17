#ifndef __LINX_RULE_SET_OUTPUT_H__
#define __LINX_RULE_SET_OUTPUT_H__

#include "linx_output_match.h"
#include "linx_event_data.h"
#include "linx_rule_engine_load.h"

/**
 * @brief 扩展的规则集合结构体
 * 
 * 在原有规则集合基础上添加预编译的输出匹配数组
 */
typedef struct {
    // 原有的规则数据
    linx_rule_t **rules;
    void **matches;  // 原有的匹配结构
    
    // 新增的输出匹配数组
    linx_output_match_t **output_matches;
    
    size_t size;
    size_t capacity;
} linx_rule_set_with_output_t;

/**
 * @brief 初始化规则集合输出系统
 * 
 * @return int 0表示成功，-1表示失败
 */
int linx_rule_set_output_init(void);

/**
 * @brief 清理规则集合输出系统
 */
void linx_rule_set_output_cleanup(void);

/**
 * @brief 为规则集合创建输出匹配
 * 
 * 为规则集合中的所有规则创建预编译的输出匹配
 * 
 * @param rule_set 规则集合指针
 * @return int 0表示成功，-1表示失败
 */
int linx_rule_set_output_create_matches(void *rule_set);

/**
 * @brief 规则匹配成功时的高效输出处理
 * 
 * 使用预编译的输出匹配快速生成并输出告警信息
 * 
 * @param rule_index 匹配的规则索引
 * @param event_data 事件数据
 * @return int 0表示成功，-1表示失败
 */
int linx_rule_set_output_handle_match(size_t rule_index, const linx_event_data_t *event_data);

/**
 * @brief 获取当前事件数据
 * 
 * 获取全局的事件数据实例（在实际应用中应该从事件队列获取）
 * 
 * @return linx_event_data_t* 事件数据指针
 */
linx_event_data_t *linx_rule_set_output_get_current_event_data(void);

/**
 * @brief 设置当前事件数据
 * 
 * 设置全局的事件数据实例
 * 
 * @param event_data 事件数据指针
 */
void linx_rule_set_output_set_current_event_data(linx_event_data_t *event_data);

/**
 * @brief 更新事件数据字段
 * 
 * 便捷函数，用于更新当前事件数据的字段
 * 
 * @param field_name 字段名称
 * @param value 字段值
 * @param is_string 是否为字符串类型
 * @return int 0表示成功，-1表示失败
 */
int linx_rule_set_output_update_event_field(const char *field_name, 
                                            const void *value, 
                                            int is_string);

#endif /* __LINX_RULE_SET_OUTPUT_H__ */