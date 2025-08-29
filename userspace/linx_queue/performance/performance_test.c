/**
 * @file performance_test.c
 * @brief 性能测试：对比原始实现和通用队列优化后的性能
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "linx_queue.h"

/* 测试配置 */
#define TEST_DURATION_SECONDS 10
#define NUM_PRODUCER_THREADS 4
#define NUM_CONSUMER_THREADS 2
#define MESSAGE_SIZE 256

/* 测试统计 */
typedef struct {
    volatile uint64_t messages_produced;
    volatile uint64_t messages_consumed;
    volatile uint64_t messages_dropped;
    volatile uint64_t total_latency_us;
    volatile uint64_t max_latency_us;
    volatile uint64_t min_latency_us;
    volatile bool running;
} test_stats_t;

/* 测试消息结构 */
typedef struct {
    uint64_t timestamp_us;
    uint32_t producer_id;
    uint32_t sequence_number;
    char data[MESSAGE_SIZE];
} test_message_t;

/* 全局变量 */
static linx_queue_t *g_test_queue = NULL;
static test_stats_t g_stats = {0};

/**
 * @brief 获取微秒时间戳
 */
static uint64_t get_timestamp_us(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000ULL + tv.tv_usec;
}

/**
 * @brief 释放测试消息
 */
static void free_test_message(void *data)
{
    free((test_message_t *)data);
}

/**
 * @brief 生产者线程
 */
static void *producer_thread(void *arg)
{
    uint32_t producer_id = *(uint32_t *)arg;
    uint32_t sequence = 0;
    test_message_t *msg;
    linx_queue_result_t result;
    
    printf("[PRODUCER_%u] Started\n", producer_id);
    
    while (g_stats.running) {
        /* 创建测试消息 */
        msg = malloc(sizeof(test_message_t));
        if (!msg) {
            __sync_add_and_fetch(&g_stats.messages_dropped, 1);
            continue;
        }
        
        msg->timestamp_us = get_timestamp_us();
        msg->producer_id = producer_id;
        msg->sequence_number = sequence++;
        snprintf(msg->data, sizeof(msg->data), 
                "Test message from producer %u, seq %u", 
                producer_id, msg->sequence_number);
        
        /* 入队 */
        result = linx_queue_push(g_test_queue, msg);
        if (result == LINX_QUEUE_OK) {
            __sync_add_and_fetch(&g_stats.messages_produced, 1);
        } else {
            __sync_add_and_fetch(&g_stats.messages_dropped, 1);
            free(msg);
        }
        
        /* 控制生产速度 */
        usleep(100); // 100微秒间隔
    }
    
    printf("[PRODUCER_%u] Stopped\n", producer_id);
    return NULL;
}

/**
 * @brief 消费者线程
 */
static void *consumer_thread(void *arg)
{
    uint32_t consumer_id = *(uint32_t *)arg;
    test_message_t *msg;
    linx_queue_result_t result;
    uint64_t current_time, latency;
    
    printf("[CONSUMER_%u] Started\n", consumer_id);
    
    while (g_stats.running) {
        /* 从队列获取消息 */
        result = linx_queue_pop_timeout(g_test_queue, (void **)&msg, 100);
        
        if (result == LINX_QUEUE_TIMEOUT) {
            continue;
        } else if (result == LINX_QUEUE_INTERRUPTED) {
            break;
        } else if (result != LINX_QUEUE_OK) {
            continue;
        }
        
        /* 计算延迟 */
        current_time = get_timestamp_us();
        latency = current_time - msg->timestamp_us;
        
        /* 更新统计信息 */
        __sync_add_and_fetch(&g_stats.messages_consumed, 1);
        __sync_add_and_fetch(&g_stats.total_latency_us, latency);
        
        /* 更新最大/最小延迟 */
        uint64_t current_max = g_stats.max_latency_us;
        while (latency > current_max) {
            if (__sync_bool_compare_and_swap(&g_stats.max_latency_us, current_max, latency)) {
                break;
            }
            current_max = g_stats.max_latency_us;
        }
        
        uint64_t current_min = g_stats.min_latency_us;
        if (current_min == 0 || latency < current_min) {
            while (current_min == 0 || latency < current_min) {
                if (__sync_bool_compare_and_swap(&g_stats.min_latency_us, current_min, latency)) {
                    break;
                }
                current_min = g_stats.min_latency_us;
            }
        }
        
        /* 释放消息 */
        free(msg);
    }
    
    printf("[CONSUMER_%u] Stopped\n", consumer_id);
    return NULL;
}

/**
 * @brief 运行性能测试
 */
void run_performance_test(const char *test_name, const linx_queue_config_t *config)
{
    pthread_t producer_threads[NUM_PRODUCER_THREADS];
    pthread_t consumer_threads[NUM_CONSUMER_THREADS];
    uint32_t producer_ids[NUM_PRODUCER_THREADS];
    uint32_t consumer_ids[NUM_CONSUMER_THREADS];
    uint64_t start_time, end_time;
    double duration, throughput, avg_latency;
    linx_queue_stats_t queue_stats;
    
    printf("\n=== %s ===\n", test_name);
    
    /* 重置统计信息 */
    memset(&g_stats, 0, sizeof(g_stats));
    g_stats.running = true;
    
    /* 创建队列 */
    g_test_queue = linx_queue_create(config);
    if (!g_test_queue) {
        printf("Failed to create queue for test: %s\n", test_name);
        return;
    }
    
    /* 启动生产者线程 */
    for (int i = 0; i < NUM_PRODUCER_THREADS; i++) {
        producer_ids[i] = i;
        if (pthread_create(&producer_threads[i], NULL, producer_thread, &producer_ids[i]) != 0) {
            printf("Failed to create producer thread %d\n", i);
            return;
        }
    }
    
    /* 启动消费者线程 */
    for (int i = 0; i < NUM_CONSUMER_THREADS; i++) {
        consumer_ids[i] = i;
        if (pthread_create(&consumer_threads[i], NULL, consumer_thread, &consumer_ids[i]) != 0) {
            printf("Failed to create consumer thread %d\n", i);
            return;
        }
    }
    
    /* 记录开始时间 */
    start_time = get_timestamp_us();
    
    /* 运行测试 */
    printf("Running test for %d seconds...\n", TEST_DURATION_SECONDS);
    sleep(TEST_DURATION_SECONDS);
    
    /* 停止测试 */
    g_stats.running = false;
    linx_queue_interrupt_all(g_test_queue);
    
    /* 记录结束时间 */
    end_time = get_timestamp_us();
    
    /* 等待所有线程结束 */
    for (int i = 0; i < NUM_PRODUCER_THREADS; i++) {
        pthread_join(producer_threads[i], NULL);
    }
    for (int i = 0; i < NUM_CONSUMER_THREADS; i++) {
        pthread_join(consumer_threads[i], NULL);
    }
    
    /* 计算统计信息 */
    duration = (end_time - start_time) / 1000000.0; // 转换为秒
    throughput = g_stats.messages_consumed / duration;
    avg_latency = g_stats.messages_consumed > 0 ? 
                 (double)g_stats.total_latency_us / g_stats.messages_consumed : 0;
    
    /* 获取队列统计信息 */
    linx_queue_get_stats(g_test_queue, &queue_stats);
    
    /* 打印结果 */
    printf("\n--- Test Results ---\n");
    printf("Duration: %.2f seconds\n", duration);
    printf("Messages produced: %lu\n", g_stats.messages_produced);
    printf("Messages consumed: %lu\n", g_stats.messages_consumed);
    printf("Messages dropped: %lu\n", g_stats.messages_dropped);
    printf("Throughput: %.0f messages/second\n", throughput);
    printf("Average latency: %.2f microseconds\n", avg_latency);
    printf("Min latency: %lu microseconds\n", g_stats.min_latency_us);
    printf("Max latency: %lu microseconds\n", g_stats.max_latency_us);
    printf("Queue capacity: %u\n", queue_stats.current_capacity);
    printf("Queue resizes: %lu\n", queue_stats.total_resizes);
    printf("Queue timeouts: %lu\n", queue_stats.total_timeouts);
    printf("Drop rate: %.2f%%\n", 
           g_stats.messages_produced > 0 ? 
           (double)g_stats.messages_dropped / g_stats.messages_produced * 100 : 0);
    
    /* 清理 */
    linx_queue_destroy(g_test_queue, free_test_message);
    g_test_queue = NULL;
}

/**
 * @brief 主函数
 */
int main(void)
{
    printf("=== Queue Performance Test ===\n");
    printf("Configuration:\n");
    printf("  Test duration: %d seconds\n", TEST_DURATION_SECONDS);
    printf("  Producer threads: %d\n", NUM_PRODUCER_THREADS);
    printf("  Consumer threads: %d\n", NUM_CONSUMER_THREADS);
    printf("  Message size: %d bytes\n", MESSAGE_SIZE);
    
    /* 测试1：小容量队列，无自动扩容 */
    linx_queue_config_t small_fixed_config = {
        .initial_capacity = 64,
        .max_capacity = 64,
        .auto_resize = false,
        .resize_factor = 0,
        .thread_safe = true,
        .use_condition = true
    };
    run_performance_test("Small Fixed Capacity Queue (64)", &small_fixed_config);
    
    /* 测试2：中等容量队列，自动扩容 */
    linx_queue_config_t medium_auto_config = {
        .initial_capacity = 256,
        .max_capacity = 0,
        .auto_resize = true,
        .resize_factor = 2,
        .thread_safe = true,
        .use_condition = true
    };
    run_performance_test("Medium Auto-Resize Queue (256->∞)", &medium_auto_config);
    
    /* 测试3：大容量队列，无自动扩容 */
    linx_queue_config_t large_fixed_config = {
        .initial_capacity = 4096,
        .max_capacity = 4096,
        .auto_resize = false,
        .resize_factor = 0,
        .thread_safe = true,
        .use_condition = true
    };
    run_performance_test("Large Fixed Capacity Queue (4096)", &large_fixed_config);
    
    /* 测试4：无锁队列（单生产者单消费者） */
    if (NUM_PRODUCER_THREADS == 1 && NUM_CONSUMER_THREADS == 1) {
        linx_queue_config_t lockfree_config = {
            .initial_capacity = 1024,
            .max_capacity = 1024,
            .auto_resize = false,
            .resize_factor = 0,
            .thread_safe = false,
            .use_condition = false
        };
        run_performance_test("Lock-Free Queue (1024)", &lockfree_config);
    }
    
    printf("\n=== Performance Test Complete ===\n");
    
    return 0;
}