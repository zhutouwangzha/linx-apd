#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "linx_hash_map_thread_safe.h"
#include "linx_hash_map.h"

#define NUM_THREADS 8
#define NUM_ITERATIONS 1000

typedef struct {
    int thread_id;
    int iterations;
} thread_data_t;

/* 测试用的结构体 */
typedef struct {
    int value1;
    char value2[64];
    double value3;
} test_struct_t;

static test_struct_t test_data[NUM_THREADS];

void *thread_worker(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    field_result_t result;
    field_update_table_t tables[1];
    
    printf("Thread %d starting\n", data->thread_id);
    
    /* 注册线程 */
    if (linx_hash_map_register_thread() != 0) {
        printf("Thread %d: Failed to register\n", data->thread_id);
        return NULL;
    }
    
    for (int i = 0; i < data->iterations; i++) {
        /* 更新线程本地的base_addr */
        test_data[data->thread_id].value1 = data->thread_id * 1000 + i;
        snprintf(test_data[data->thread_id].value2, sizeof(test_data[data->thread_id].value2), 
                 "thread_%d_iter_%d", data->thread_id, i);
        test_data[data->thread_id].value3 = data->thread_id * 3.14 + i * 0.01;
        
        tables[0].table_name = "test_table";
        tables[0].base_addr = &test_data[data->thread_id];
        
        if (linx_hash_map_thread_local_update_tables_base(tables, 1) != 0) {
            printf("Thread %d: Failed to update base addr at iteration %d\n", data->thread_id, i);
            continue;
        }
        
        /* 测试字段查询 */
        result = linx_hash_map_thread_local_get_field("test_table", "value1");
        if (!result.found) {
            printf("Thread %d: Field not found at iteration %d\n", data->thread_id, i);
            continue;
        }
        
        /* 验证数据正确性 */
        void *base_addr = linx_hash_map_thread_local_get_table_base("test_table");
        if (base_addr != &test_data[data->thread_id]) {
            printf("Thread %d: Base address mismatch at iteration %d\n", data->thread_id, i);
        }
        
        test_struct_t *struct_ptr = (test_struct_t *)base_addr;
        if (struct_ptr->value1 != data->thread_id * 1000 + i) {
            printf("Thread %d: Value mismatch at iteration %d: expected %d, got %d\n", 
                   data->thread_id, i, data->thread_id * 1000 + i, struct_ptr->value1);
        }
        
        /* 模拟一些处理时间 */
        usleep(10);
    }
    
    printf("Thread %d completed successfully\n", data->thread_id);
    
    /* 注销线程 */
    linx_hash_map_unregister_thread();
    
    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    int ret;
    
    printf("Testing thread-safe hash map with %d threads, %d iterations each\n", 
           NUM_THREADS, NUM_ITERATIONS);
    
    /* 初始化全局hash map */
    ret = linx_hash_map_init();
    if (ret) {
        printf("Failed to initialize global hash map\n");
        return -1;
    }
    
    /* 初始化线程安全hash map */
    ret = linx_hash_map_thread_safe_init();
    if (ret) {
        printf("Failed to initialize thread-safe hash map\n");
        return -1;
    }
    
    /* 创建测试表 */
    ret = linx_hash_map_create_table("test_table", NULL);
    if (ret) {
        printf("Failed to create test table\n");
        return -1;
    }
    
    /* 添加字段映射 */
    ret = linx_hash_map_add_field("test_table", "value1", 
                                  offsetof(test_struct_t, value1), 
                                  sizeof(int), LINX_FIELD_TYPE_INT32);
    if (ret) {
        printf("Failed to add field value1\n");
        return -1;
    }
    
    ret = linx_hash_map_add_field("test_table", "value2", 
                                  offsetof(test_struct_t, value2), 
                                  sizeof(((test_struct_t *)0)->value2), 
                                  LINX_FIELD_TYPE_CHARBUF);
    if (ret) {
        printf("Failed to add field value2\n");
        return -1;
    }
    
    ret = linx_hash_map_add_field("test_table", "value3", 
                                  offsetof(test_struct_t, value3), 
                                  sizeof(double), LINX_FIELD_TYPE_DOUBLE);
    if (ret) {
        printf("Failed to add field value3\n");
        return -1;
    }
    
    /* 创建并启动线程 */
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].iterations = NUM_ITERATIONS;
        
        ret = pthread_create(&threads[i], NULL, thread_worker, &thread_data[i]);
        if (ret) {
            printf("Failed to create thread %d\n", i);
            return -1;
        }
    }
    
    /* 等待所有线程完成 */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("All threads completed successfully!\n");
    
    /* 清理资源 */
    linx_hash_map_thread_safe_deinit();
    linx_hash_map_deinit();
    
    return 0;
}