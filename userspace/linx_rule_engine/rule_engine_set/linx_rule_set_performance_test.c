#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "linx_rule_engine_set.h"
#include "linx_event_classifier.h"
#include "linx_rule_analyzer.h"

/**
 * 性能测试程序
 * 用于对比优化前后的规则匹配效率
 */

static void print_stats(void)
{
    struct {
        size_t total_rules;
        size_t indexed_rules;
        size_t generic_rules;
        size_t match_attempts;
        size_t successful_matches;
        size_t skipped_rules;
    } stats;
    
    if (linx_rule_set_get_stats(&stats) == 0) {
        printf("\n=== 规则集合统计信息 ===\n");
        printf("总规则数: %zu\n", stats.total_rules);
        printf("索引规则数: %zu\n", stats.indexed_rules);
        printf("通用规则数: %zu\n", stats.generic_rules);
        printf("匹配尝试次数: %zu\n", stats.match_attempts);
        printf("成功匹配次数: %zu\n", stats.successful_matches);
        printf("跳过的规则数: %zu\n", stats.skipped_rules);
        
        if (stats.match_attempts > 0) {
            double efficiency = (double)stats.skipped_rules / stats.match_attempts * 100.0;
            printf("过滤效率: %.2f%%\n", efficiency);
        }
        printf("========================\n\n");
    }
}

static void create_test_events(linx_event_t *events, size_t count)
{
    /* 创建测试事件 */
    const uint32_t test_types[] = {
        LINX_EVENT_TYPE_OPEN_E,
        LINX_EVENT_TYPE_READ_E,
        LINX_EVENT_TYPE_WRITE_E,
        LINX_EVENT_TYPE_CLOSE_E,
        LINX_EVENT_TYPE_EXECVE_E,
        LINX_EVENT_TYPE_SOCKET_E,
        LINX_EVENT_TYPE_CONNECT_E,
        LINX_EVENT_TYPE_SENDTO_E,
        LINX_EVENT_TYPE_FORK_E,
        LINX_EVENT_TYPE_EXIT_E
    };
    
    size_t type_count = sizeof(test_types) / sizeof(test_types[0]);
    
    for (size_t i = 0; i < count; i++) {
        memset(&events[i], 0, sizeof(linx_event_t));
        events[i].type = test_types[i % type_count];
        events[i].tid = 1000 + i;
        events[i].pid = 1000 + i;
        events[i].time = time(NULL);
        snprintf(events[i].comm, sizeof(events[i].comm), "test_proc_%zu", i);
    }
}

static void performance_test(size_t event_count)
{
    printf("开始性能测试 (事件数量: %zu)...\n", event_count);
    
    /* 创建测试事件 */
    linx_event_t *events = malloc(event_count * sizeof(linx_event_t));
    if (!events) {
        printf("内存分配失败\n");
        return;
    }
    
    create_test_events(events, event_count);
    
    /* 重置统计信息 */
    linx_rule_set_reset_stats();
    
    /* 测试匹配性能 */
    clock_t start = clock();
    
    for (size_t i = 0; i < event_count; i++) {
        linx_rule_set_match_event(&events[i]);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("匹配完成，用时: %.6f 秒\n", elapsed);
    printf("平均每个事件匹配时间: %.6f 毫秒\n", elapsed * 1000.0 / event_count);
    
    print_stats();
    
    free(events);
}

static void classification_test(void)
{
    printf("\n=== 事件分类测试 ===\n");
    
    linx_event_t test_event;
    linx_event_classification_t classification;
    
    /* 测试不同类型的事件分类 */
    const struct {
        uint32_t type;
        const char *name;
    } test_cases[] = {
        {LINX_EVENT_TYPE_OPEN_E, "OPEN_ENTER"},
        {LINX_EVENT_TYPE_OPEN_X, "OPEN_EXIT"},
        {LINX_EVENT_TYPE_SOCKET_E, "SOCKET_ENTER"},
        {LINX_EVENT_TYPE_EXECVE_E, "EXECVE_ENTER"},
        {LINX_EVENT_TYPE_MMAP_E, "MMAP_ENTER"},
        {LINX_EVENT_TYPE_KILL_E, "KILL_ENTER"}
    };
    
    size_t case_count = sizeof(test_cases) / sizeof(test_cases[0]);
    
    for (size_t i = 0; i < case_count; i++) {
        memset(&test_event, 0, sizeof(test_event));
        test_event.type = test_cases[i].type;
        
        if (linx_event_classify(&test_event, &classification) == 0) {
            printf("事件 %s:\n", test_cases[i].name);
            printf("  源: %s\n", linx_event_source_to_string(classification.source));
            printf("  方向: %s\n", linx_event_direction_to_string(classification.direction));
            printf("  类型: %u\n", classification.type);
        } else {
            printf("事件 %s 分类失败\n", test_cases[i].name);
        }
    }
    printf("===================\n\n");
}

int main(int argc, char *argv[])
{
    printf("LINX 规则集合优化性能测试\n");
    printf("==========================\n\n");
    
    /* 初始化系统 */
    if (linx_rule_set_init() != 0) {
        printf("规则集合初始化失败\n");
        return 1;
    }
    
    if (linx_rule_analyzer_init() != 0) {
        printf("规则分析器初始化失败\n");
        linx_rule_set_deinit();
        return 1;
    }
    
    /* 事件分类测试 */
    classification_test();
    
    /* 性能测试 */
    performance_test(1000);
    performance_test(10000);
    
    /* 清理 */
    linx_rule_analyzer_deinit();
    linx_rule_set_deinit();
    
    printf("测试完成\n");
    return 0;
}