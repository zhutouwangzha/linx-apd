/**
 * 多线程规则匹配示例程序
 * 
 * 编译：
 * gcc -o rule_match_mt_example rule_match_mt_example.c -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

/* 模拟的配置结构 */
typedef struct {
    int enable_mt_match;
    int num_match_threads;
} config_t;

/* 模拟的规则匹配时间（毫秒） */
#define RULE_MATCH_TIME_MS 10
#define NUM_RULES 1000
#define NUM_EVENTS 100

/* 全局配置 */
static config_t g_config = {
    .enable_mt_match = 1,
    .num_match_threads = 4
};

/* 模拟单线程规则匹配 */
void single_thread_match(int num_rules)
{
    for (int i = 0; i < num_rules; i++) {
        usleep(RULE_MATCH_TIME_MS * 1000);
    }
}

/* 模拟多线程规则匹配 */
void *match_worker(void *arg)
{
    int *rules = (int *)arg;
    int num_rules = *rules;
    
    for (int i = 0; i < num_rules; i++) {
        usleep(RULE_MATCH_TIME_MS * 1000);
    }
    
    return NULL;
}

void multi_thread_match(int num_rules, int num_threads)
{
    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);
    int *thread_rules = malloc(sizeof(int) * num_threads);
    int rules_per_thread = num_rules / num_threads;
    int remaining = num_rules % num_threads;
    
    /* 创建工作线程 */
    for (int i = 0; i < num_threads; i++) {
        thread_rules[i] = rules_per_thread;
        if (i == num_threads - 1) {
            thread_rules[i] += remaining;
        }
        pthread_create(&threads[i], NULL, match_worker, &thread_rules[i]);
    }
    
    /* 等待所有线程完成 */
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    free(threads);
    free(thread_rules);
}

/* 获取当前时间（毫秒） */
long long get_time_ms()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int main()
{
    long long start_time, end_time;
    double single_thread_time, multi_thread_time;
    
    printf("规则匹配性能测试\n");
    printf("规则数: %d\n", NUM_RULES);
    printf("事件数: %d\n", NUM_EVENTS);
    printf("线程数: %d\n\n", g_config.num_match_threads);
    
    /* 单线程测试 */
    printf("开始单线程测试...\n");
    start_time = get_time_ms();
    
    for (int i = 0; i < NUM_EVENTS; i++) {
        single_thread_match(NUM_RULES);
    }
    
    end_time = get_time_ms();
    single_thread_time = (end_time - start_time) / 1000.0;
    printf("单线程完成时间: %.2f 秒\n\n", single_thread_time);
    
    /* 多线程测试 */
    printf("开始多线程测试...\n");
    start_time = get_time_ms();
    
    for (int i = 0; i < NUM_EVENTS; i++) {
        multi_thread_match(NUM_RULES, g_config.num_match_threads);
    }
    
    end_time = get_time_ms();
    multi_thread_time = (end_time - start_time) / 1000.0;
    printf("多线程完成时间: %.2f 秒\n\n", multi_thread_time);
    
    /* 性能对比 */
    printf("性能提升: %.2fx\n", single_thread_time / multi_thread_time);
    printf("效率: %.2f%%\n", (single_thread_time / multi_thread_time) / g_config.num_match_threads * 100);
    
    return 0;
}