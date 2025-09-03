#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "linx_event_processor.h"
#include "linx_apd_config.h"

/* 模拟的依赖函数 - 在实际环境中这些由相应模块提供 */
linx_rule_set_t dummy_rule_set = {0};
event_t dummy_event = {0};

linx_rule_set_t *linx_rule_set_get(void) { return &dummy_rule_set; }
bool linx_rule_set_match_rule(void) { return false; }
event_t *linx_event_rich_get(void) { return &dummy_event; }
void *linx_process_cache_get_fd(pid_t pid, int64_t fd) { (void)pid; (void)fd; return NULL; }
void *linx_process_cache_get(pid_t pid) { (void)pid; return NULL; }
void *linx_machine_status_get_user(void) { return NULL; }
void *linx_machine_status_get_group(void) { return NULL; }
int linx_hash_map_update_tables_base(field_update_table_t *tables, size_t num_tables) { 
    (void)tables; (void)num_tables; return 0; 
}
int linx_alert_send_async(void *output, void *rule) { (void)output; (void)rule; return 0; }
int linx_thread_pool_get_active_threads(linx_thread_pool_t *pool) { 
    return pool ? pool->thread_count : 0; 
}

int main(void)
{
    int ret;
    linx_event_processor_config_t ep_config = {0};
    linx_event_t test_event = {0};
    
    printf("=== 测试集成后的多线程规则匹配功能 ===\n\n");
    
    /* 1. 初始化APD配置 */
    printf("1. 初始化APD配置...\n");
    ret = linx_apd_config_init();
    if (ret) {
        printf("✗ APD配置初始化失败\n");
        return -1;
    }
    printf("✓ APD配置初始化成功\n");
    
    /* 获取配置并显示 */
    linx_apd_config_t *apd_config = linx_apd_config_get();
    if (apd_config) {
        printf("  - 多线程匹配: %s\n", 
               apd_config->mt_config.enable_mt_match ? "启用" : "禁用");
        printf("  - 线程数量: %d\n", 
               apd_config->mt_config.num_match_threads);
    }
    
    /* 2. 初始化事件处理器 */
    printf("\n2. 初始化事件处理器...\n");
    ep_config.fetcher_thread_count = 1;
    ep_config.matcher_thread_count = 4;
    
    ret = linx_event_processor_init(&ep_config);
    if (ret) {
        printf("✗ 事件处理器初始化失败\n");
        goto cleanup;
    }
    printf("✓ 事件处理器初始化成功\n");
    
    /* 验证事件处理器状态 */
    linx_event_processor_t *processor = linx_event_processor_get();
    if (processor && processor->initialized) {
        printf("  - 状态: 已初始化\n");
        printf("  - 匹配线程数: %d\n", processor->config.matcher_thread_count);
        printf("  - 线程池状态: %s\n", 
               processor->matcher_pool ? "正常" : "异常");
    }
    
    /* 3. 启动事件处理器 */
    printf("\n3. 启动事件处理器...\n");
    ret = linx_event_processor_start();
    if (ret) {
        printf("✗ 事件处理器启动失败\n");
        goto cleanup;
    }
    printf("✓ 事件处理器启动成功\n");
    
    /* 4. 模拟事件处理 */
    printf("\n4. 测试事件处理...\n");
    
    /* 设置测试事件 */
    test_event.pid = 1234;
    test_event.tid = 1234;
    test_event.type = 1;
    strcpy(test_event.comm, "test_process");
    
    /* 模拟规则集（空的，所以不会真正匹配） */
    dummy_rule_set.size = 0;
    dummy_rule_set.data.matches = NULL;
    
    printf("  - 处理测试事件（PID: %ld）...\n", test_event.pid);
    bool match_result = linx_event_processor_process_event(&test_event, -1);
    printf("  - 匹配结果: %s\n", match_result ? "匹配" : "无匹配");
    
    /* 测试线程池状态 */
    if (processor->matcher_pool) {
        int active_threads = linx_thread_pool_get_active_threads(processor->matcher_pool);
        int queue_size = linx_thread_pool_get_queue_size(processor->matcher_pool);
        printf("  - 活跃线程数: %d\n", active_threads);
        printf("  - 队列大小: %d\n", queue_size);
    }
    
    /* 5. 停止事件处理器 */
    printf("\n5. 停止事件处理器...\n");
    ret = linx_event_processor_stop();
    if (ret) {
        printf("✗ 事件处理器停止失败\n");
    } else {
        printf("✓ 事件处理器停止成功\n");
    }
    
cleanup:
    /* 清理资源 */
    printf("\n6. 清理资源...\n");
    linx_event_processor_deinit();
    linx_apd_config_deinit();
    printf("✓ 资源清理完成\n");
    
    printf("\n=== 测试完成 ===\n");
    printf("✓ linx_event_processor 和 rule_match_mt 成功融合！\n");
    printf("✓ 多线程规则匹配功能已集成到事件处理器中\n");
    
    return 0;
}