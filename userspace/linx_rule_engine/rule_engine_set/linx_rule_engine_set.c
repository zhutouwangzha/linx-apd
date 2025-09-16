#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "linx_rule_engine_set.h"
#include "linx_alert.h"
#include "linx_event_classifier.h"

static linx_rule_set_t *rule_set = NULL;

/* 辅助函数声明 */
static int linx_rule_list_init(linx_rule_list_t **list);
static void linx_rule_list_destroy(linx_rule_list_t *list);
static int linx_rule_list_add(linx_rule_list_t *list, linx_rule_entry_t *entry);
static linx_rule_list_t *linx_rule_set_get_rule_list(const linx_event_classification_t *classification);
static bool linx_rule_set_match_rule_list(linx_rule_list_t *list, const linx_event_t *event);

int linx_rule_set_init(void)
{
    if (rule_set != NULL) {
        return 0; /* 已经初始化 */
    }
    
    rule_set = malloc(sizeof(linx_rule_set_t));
    if (rule_set == NULL) {
        return -1;
    }
    
    memset(rule_set, 0, sizeof(linx_rule_set_t));
    
    /* 初始化事件分类器 */
    if (linx_event_classifier_init() != 0) {
        free(rule_set);
        rule_set = NULL;
        return -1;
    }
    
    /* 分配三级索引数组 */
    rule_set->indexed_rules = malloc(LINX_EVENT_SOURCE_MAX * sizeof(linx_rule_list_t **));
    if (rule_set->indexed_rules == NULL) {
        linx_event_classifier_deinit();
        free(rule_set);
        rule_set = NULL;
        return -1;
    }
    
    for (int i = 0; i < LINX_EVENT_SOURCE_MAX; i++) {
        rule_set->indexed_rules[i] = malloc(LINX_EVENT_DIRECTION_MAX * sizeof(linx_rule_list_t *));
        if (rule_set->indexed_rules[i] == NULL) {
            /* 清理已分配的内存 */
            for (int j = 0; j < i; j++) {
                free(rule_set->indexed_rules[j]);
            }
            free(rule_set->indexed_rules);
            linx_event_classifier_deinit();
            free(rule_set);
            rule_set = NULL;
            return -1;
        }
        
        for (int j = 0; j < LINX_EVENT_DIRECTION_MAX; j++) {
            rule_set->indexed_rules[i][j] = NULL;
        }
    }
    
    /* 初始化通用规则列表 */
    if (linx_rule_list_init(&rule_set->generic_rules) != 0) {
        for (int i = 0; i < LINX_EVENT_SOURCE_MAX; i++) {
            free(rule_set->indexed_rules[i]);
        }
        free(rule_set->indexed_rules);
        linx_event_classifier_deinit();
        free(rule_set);
        rule_set = NULL;
        return -1;
    }
    
    rule_set->initialized = true;
    return 0;
}

void linx_rule_set_deinit(void)
{
    if (!rule_set) {
        return;
    }

    /* 清理索引规则 */
    if (rule_set->indexed_rules) {
        for (int i = 0; i < LINX_EVENT_SOURCE_MAX; i++) {
            if (rule_set->indexed_rules[i]) {
                for (int j = 0; j < LINX_EVENT_DIRECTION_MAX; j++) {
                    if (rule_set->indexed_rules[i][j]) {
                        linx_rule_list_destroy(rule_set->indexed_rules[i][j]);
                    }
                }
                free(rule_set->indexed_rules[i]);
            }
        }
        free(rule_set->indexed_rules);
    }
    
    /* 清理通用规则 */
    if (rule_set->generic_rules) {
        linx_rule_list_destroy(rule_set->generic_rules);
    }
    
    /* 清理事件分类器 */
    linx_event_classifier_deinit();

    free(rule_set);
    rule_set = NULL;
}

linx_rule_set_t *linx_rule_set_get(void)
{
    return rule_set;
}

/* 辅助函数实现 */
static int linx_rule_list_init(linx_rule_list_t **list)
{
    *list = malloc(sizeof(linx_rule_list_t));
    if (*list == NULL) {
        return -1;
    }
    
    (*list)->entries = NULL;
    (*list)->size = 0;
    (*list)->capacity = 0;
    
    return 0;
}

static void linx_rule_list_destroy(linx_rule_list_t *list)
{
    if (!list) {
        return;
    }
    
    for (size_t i = 0; i < list->size; i++) {
        if (list->entries[i]) {
            linx_rule_destroy(list->entries[i]->rule);
            linx_rule_engine_match_destroy(list->entries[i]->match);
            linx_output_match_destroy(list->entries[i]->output);
            free(list->entries[i]);
        }
    }
    
    free(list->entries);
    free(list);
}

static int linx_rule_list_resize(linx_rule_list_t *list)
{
    size_t new_capacity = list->capacity == 0 ? 16 : list->capacity * 2;
    linx_rule_entry_t **new_entries = realloc(list->entries, 
                                               new_capacity * sizeof(linx_rule_entry_t *));
    if (new_entries == NULL) {
        return -1;
    }
    
    list->entries = new_entries;
    list->capacity = new_capacity;
    return 0;
}

static int linx_rule_list_add(linx_rule_list_t *list, linx_rule_entry_t *entry)
{
    if (!list || !entry) {
        return -1;
    }
    
    if (list->size >= list->capacity) {
        if (linx_rule_list_resize(list) != 0) {
            return -1;
        }
    }
    
    list->entries[list->size++] = entry;
    return 0;
}

static linx_rule_list_t *linx_rule_set_get_rule_list(const linx_event_classification_t *classification)
{
    if (!classification || !rule_set || !rule_set->initialized) {
        return rule_set ? rule_set->generic_rules : NULL;
    }
    
    linx_event_source_t source = classification->source;
    linx_event_direction_t direction = classification->direction;
    
    if (source >= LINX_EVENT_SOURCE_MAX || direction >= LINX_EVENT_DIRECTION_MAX) {
        return rule_set->generic_rules;
    }
    
    /* 如果对应的规则列表不存在，创建它 */
    if (rule_set->indexed_rules[source][direction] == NULL) {
        if (linx_rule_list_init(&rule_set->indexed_rules[source][direction]) != 0) {
            return rule_set->generic_rules;
        }
    }
    
    return rule_set->indexed_rules[source][direction];
}

int linx_rule_set_add(linx_rule_t *rule, linx_rule_match_t *match, linx_output_match_t *output, 
                      const linx_event_classification_t *classification)
{
    if (!rule_set || !rule || !match || !output) {
        return -1;
    }
    
    if (!rule_set->initialized) {
        return -1;
    }
    
    /* 创建规则条目 */
    linx_rule_entry_t *entry = malloc(sizeof(linx_rule_entry_t));
    if (!entry) {
        return -1;
    }
    
    entry->rule = rule;
    entry->match = match;
    entry->output = output;
    
    if (classification) {
        entry->classification = *classification;
    } else {
        /* 通用规则，使用默认分类 */
        entry->classification.source = LINX_EVENT_SOURCE_UNKNOWN;
        entry->classification.direction = LINX_EVENT_DIRECTION_ENTER;
        entry->classification.type = 0;
    }
    
    /* 获取对应的规则列表 */
    linx_rule_list_t *list = linx_rule_set_get_rule_list(classification);
    if (!list) {
        free(entry);
        return -1;
    }
    
    /* 添加到规则列表 */
    if (linx_rule_list_add(list, entry) != 0) {
        free(entry);
        return -1;
    }
    
    /* 更新统计信息 */
    rule_set->stats.total_rules++;
    if (classification && classification->source != LINX_EVENT_SOURCE_UNKNOWN) {
        rule_set->stats.indexed_rules++;
    } else {
        rule_set->stats.generic_rules++;
    }
    
    return 0;
}

static bool linx_rule_set_match_rule_list(linx_rule_list_t *list, const linx_event_t *event)
{
    if (!list || !event) {
        return false;
    }
    
    bool match = false;
    
    for (size_t i = 0; i < list->size; i++) {
        linx_rule_entry_t *entry = list->entries[i];
        if (!entry || !entry->match) {
            continue;
        }
        
        rule_set->stats.match_attempts++;
        
        /* 设置事件上下文 */
        linx_rule_engine_match_set_base(entry->match, (void *)event);
        
        if (entry->match->func(entry->match->context)) {
            match = true;
            rule_set->stats.successful_matches++;
            
            linx_alert_send_async(entry->output, entry->rule);
            
            /**
             * 这里有一个yaml配置可以控制匹配到规则后是否继续匹配后面的规则
             * 计划在后续添加 
             * 这里还要添加一个匹配成功然后输出消息的逻辑
             */
            break;
        }
    }
    
    return match;
}

bool linx_rule_set_match_event(const linx_event_t *event)
{
    if (!rule_set || !rule_set->initialized || !event) {
        return false;
    }
    
    bool match = false;
    
    /* 对事件进行分类 */
    linx_event_classification_t classification;
    if (linx_event_classify(event, &classification) != 0) {
        /* 分类失败，只匹配通用规则 */
        return linx_rule_set_match_rule_list(rule_set->generic_rules, event);
    }
    
    /* 首先匹配特定的索引规则 */
    linx_rule_list_t *indexed_list = NULL;
    if (classification.source < LINX_EVENT_SOURCE_MAX && 
        classification.direction < LINX_EVENT_DIRECTION_MAX &&
        rule_set->indexed_rules[classification.source][classification.direction] != NULL) {
        
        indexed_list = rule_set->indexed_rules[classification.source][classification.direction];
        match = linx_rule_set_match_rule_list(indexed_list, event);
        
        if (match) {
            return true; /* 找到匹配的规则，直接返回 */
        }
    }
    
    /* 如果没有找到匹配的索引规则，检查通用规则 */
    if (rule_set->generic_rules) {
        /* 计算跳过的规则数量（用于性能统计） */
        size_t skipped = 0;
        for (int i = 0; i < LINX_EVENT_SOURCE_MAX; i++) {
            for (int j = 0; j < LINX_EVENT_DIRECTION_MAX; j++) {
                if (rule_set->indexed_rules[i][j] && 
                    (i != classification.source || j != classification.direction)) {
                    skipped += rule_set->indexed_rules[i][j]->size;
                }
            }
        }
        rule_set->stats.skipped_rules += skipped;
        
        match = linx_rule_set_match_rule_list(rule_set->generic_rules, event);
    }
    
    return match;
}

bool linx_rule_set_match_rule(void)
{
    /* 兼容性接口 - 当没有具体事件时，匹配所有通用规则 */
    if (!rule_set || !rule_set->initialized) {
        return false;
    }
    
    /* 创建一个虚拟事件用于兼容性匹配 */
    linx_event_t dummy_event = {0};
    return linx_rule_set_match_rule_list(rule_set->generic_rules, &dummy_event);
}

int linx_rule_set_get_stats(struct {
    size_t total_rules;
    size_t indexed_rules;
    size_t generic_rules;
    size_t match_attempts;
    size_t successful_matches;
    size_t skipped_rules;
} *stats)
{
    if (!stats || !rule_set) {
        return -1;
    }
    
    stats->total_rules = rule_set->stats.total_rules;
    stats->indexed_rules = rule_set->stats.indexed_rules;
    stats->generic_rules = rule_set->stats.generic_rules;
    stats->match_attempts = rule_set->stats.match_attempts;
    stats->successful_matches = rule_set->stats.successful_matches;
    stats->skipped_rules = rule_set->stats.skipped_rules;
    
    return 0;
}

void linx_rule_set_reset_stats(void)
{
    if (!rule_set) {
        return;
    }
    
    rule_set->stats.match_attempts = 0;
    rule_set->stats.successful_matches = 0;
    rule_set->stats.skipped_rules = 0;
}
