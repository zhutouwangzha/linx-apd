#ifndef __LINX_RULE_ENGINE_SET_H__
#define __LINX_RULE_ENGINE_SET_H__ 

#include <stddef.h>
#include <stdbool.h>

#include "linx_rule_engine_load.h"
#include "linx_rule_engine_match.h"
#include "linx_event_classifier.h"

/**
 * 单个规则条目
 */
typedef struct {
    linx_rule_t *rule;
    linx_rule_match_t *match;
    linx_output_match_t *output;
    linx_event_classification_t classification;  /* 规则适用的事件分类 */
} linx_rule_entry_t;

/**
 * 规则列表 - 存储具有相同分类的规则
 */
typedef struct {
    linx_rule_entry_t **entries;
    size_t size;
    size_t capacity;
} linx_rule_list_t;

/**
 * 三级索引的规则集合
 * 第一级：事件源 (source)
 * 第二级：事件方向 (direction) 
 * 第三级：事件类型 (type)
 */
typedef struct {
    /* 三级索引: [source][direction][type] */
    linx_rule_list_t ***indexed_rules;
    
    /* 通用规则列表 - 不依赖特定事件分类的规则 */
    linx_rule_list_t *generic_rules;
    
    /* 统计信息 */
    struct {
        size_t total_rules;
        size_t indexed_rules;
        size_t generic_rules;
        size_t match_attempts;
        size_t successful_matches;
        size_t skipped_rules;  /* 通过分类过滤跳过的规则数 */
    } stats;
    
    /* 是否已初始化 */
    bool initialized;
} linx_rule_set_t;

int linx_rule_set_init(void);

void linx_rule_set_deinit(void);

linx_rule_set_t *linx_rule_set_get(void);

/**
 * 添加规则到规则集合
 * @param rule 规则定义
 * @param match 规则匹配器
 * @param output 输出匹配器
 * @param classification 事件分类信息 (可选，NULL表示通用规则)
 * @return 成功返回0，失败返回-1
 */
int linx_rule_set_add(linx_rule_t *rule, linx_rule_match_t *match, linx_output_match_t *output, 
                      const linx_event_classification_t *classification);

/**
 * 针对特定事件进行规则匹配
 * @param event 要匹配的事件
 * @return 匹配成功返回true，否则返回false
 */
bool linx_rule_set_match_event(const linx_event_t *event);

/**
 * 传统的规则匹配接口 (兼容性)
 * @return 匹配成功返回true，否则返回false
 */
bool linx_rule_set_match_rule(void);

/**
 * 获取规则集合统计信息
 * @param stats 输出统计信息
 * @return 成功返回0，失败返回-1
 */
int linx_rule_set_get_stats(struct {
    size_t total_rules;
    size_t indexed_rules;
    size_t generic_rules;
    size_t match_attempts;
    size_t successful_matches;
    size_t skipped_rules;
} *stats);

/**
 * 重置统计信息
 */
void linx_rule_set_reset_stats(void);

#endif /* __LINX_RULE_ENGINE_SET_H__ */
