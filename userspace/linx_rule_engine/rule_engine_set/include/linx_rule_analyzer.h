#ifndef __LINX_RULE_ANALYZER_H__
#define __LINX_RULE_ANALYZER_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "linx_rule_engine_load.h"
#include "linx_event_classifier.h"

/**
 * 规则分析结果
 */
typedef struct {
    bool has_classification;                    /* 是否能够确定分类 */
    linx_event_classification_t classification; /* 事件分类 */
    bool is_generic;                           /* 是否为通用规则 */
} linx_rule_analysis_t;

/**
 * 初始化规则分析器
 * @return 成功返回0，失败返回-1
 */
int linx_rule_analyzer_init(void);

/**
 * 清理规则分析器
 */
void linx_rule_analyzer_deinit(void);

/**
 * 分析规则条件，提取事件分类信息
 * @param rule 要分析的规则
 * @param analysis 输出的分析结果
 * @return 成功返回0，失败返回-1
 */
int linx_rule_analyze(const linx_rule_t *rule, linx_rule_analysis_t *analysis);

/**
 * 从条件字符串中提取事件类型
 * @param condition 条件字符串
 * @param event_types 输出的事件类型数组
 * @param max_types 最大事件类型数量
 * @return 提取到的事件类型数量，失败返回-1
 */
int linx_rule_extract_event_types(const char *condition, uint32_t *event_types, size_t max_types);

/**
 * 从条件字符串中提取事件方向
 * @param condition 条件字符串
 * @return 事件方向，无法确定返回LINX_EVENT_DIRECTION_MAX
 */
linx_event_direction_t linx_rule_extract_direction(const char *condition);

/**
 * 判断规则是否为通用规则
 * @param condition 条件字符串
 * @return 是通用规则返回true，否则返回false
 */
bool linx_rule_is_generic(const char *condition);

#endif /* __LINX_RULE_ANALYZER_H__ */