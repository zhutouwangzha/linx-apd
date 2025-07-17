#ifndef __LINX_OUTPUT_INTEGRATION_H__
#define __LINX_OUTPUT_INTEGRATION_H__

#include "linx_output_match.h"
#include "linx_event_data.h"
#include "linx_rule_engine_load.h"

/**
 * @brief 扩展的规则结构体
 * 
 * 在原有规则结构体基础上添加预编译的输出匹配
 */
typedef struct {
    linx_rule_t base_rule;              // 基础规则信息
    linx_output_match_t *output_match;  // 预编译的输出匹配
} linx_rule_with_output_t;

/**
 * @brief 初始化输出集成模块
 * 
 * @return int 0表示成功，-1表示失败
 */
int linx_output_integration_init(void);

/**
 * @brief 清理输出集成模块
 */
void linx_output_integration_cleanup(void);

/**
 * @brief 为规则创建输出匹配
 * 
 * 根据规则的output字段创建预编译的输出匹配结构
 * 
 * @param rule 规则结构体
 * @param output_match 输出匹配结构体指针的指针
 * @return int 0表示成功，-1表示失败
 */
int linx_output_integration_create_match_for_rule(const linx_rule_t *rule, 
                                                  linx_output_match_t **output_match);

/**
 * @brief 使用规则和事件数据快速生成输出
 * 
 * @param rule 规则结构体
 * @param event_data 事件数据
 * @return int 0表示成功，-1表示失败
 */
int linx_output_integration_format_and_print(const linx_rule_t *rule,
                                             const linx_event_data_t *event_data);

/**
 * @brief 批量为规则集合创建输出匹配
 * 
 * @param rules 规则数组
 * @param rule_count 规则数量
 * @param output_matches 输出匹配数组（需要调用者分配）
 * @return int 成功创建的输出匹配数量，失败返回-1
 */
int linx_output_integration_create_matches_for_rules(const linx_rule_t *rules,
                                                     size_t rule_count,
                                                     linx_output_match_t **output_matches);

/**
 * @brief 注册默认的字段绑定
 * 
 * 为常用的字段（如%evt.time, %proc.pid等）注册默认绑定
 * 
 * @param output_match 输出匹配结构体
 * @return int 0表示成功，-1表示失败
 */
int linx_output_integration_register_default_bindings(linx_output_match_t *output_match);

#endif /* __LINX_OUTPUT_INTEGRATION_H__ */