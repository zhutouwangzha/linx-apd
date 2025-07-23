#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <pthread.h>
#include <unistd.h>
#include "linx_hash_map.h"

typedef struct {
    int id;
    char name[32];
    float score;
} student_t;

/* 全局变量 */
static student_t *g_student = NULL;
static volatile bool g_stop = false;

/* 线程1：读取数据 */
void *reader_thread(void *arg)
{
    int thread_id = *(int*)arg;
    
    printf("读取线程 %d 启动\n", thread_id);
    
    while (!g_stop) {
        /* === 方法1：使用线程安全API === */
        access_context_t context = linx_hash_map_lock_table_read("student");
        if (context.locked) {
            field_result_t field;
            
            /* 在锁保护期间，基地址不会改变 */
            field = linx_hash_map_get_field("student", "id");
            if (field.found) {
                int *id_ptr = (int *)linx_hash_map_get_value_ptr_safe(&context, &field);
                if (id_ptr) {
                    printf("线程%d [安全] student.id = %d (基地址: %p)\n", 
                           thread_id, *id_ptr, context.base_addr);
                }
            }
            
            field = linx_hash_map_get_field("student", "name");
            if (field.found) {
                char *name_ptr = (char *)linx_hash_map_get_value_ptr_safe(&context, &field);
                if (name_ptr) {
                    printf("线程%d [安全] student.name = %s\n", thread_id, name_ptr);
                }
            }
            
            /* 释放锁 */
            linx_hash_map_unlock_context(&context);
        }
        
        usleep(100000); // 100ms
        
        /* === 方法2：演示非线程安全的问题 === */
        field_result_t field = linx_hash_map_get_field("student", "score");
        if (field.found) {
            /* 这里可能存在竞态条件 */
            usleep(10000); // 模拟处理延迟，增加竞态条件发生概率
            
            float *score_ptr = (float *)linx_hash_map_get_table_value_ptr("student", &field);
            if (score_ptr) {
                printf("线程%d [非安全] student.score = %.1f\n", thread_id, *score_ptr);
            }
        }
        
        usleep(200000); // 200ms
    }
    
    printf("读取线程 %d 结束\n", thread_id);
    return NULL;
}

/* 线程2：更新基地址 */
void *updater_thread(void *arg)
{
    printf("更新线程启动\n");
    
    student_t *old_student = g_student;
    
    for (int i = 0; i < 10 && !g_stop; i++) {
        sleep(1); // 1秒更新一次
        
        /* 分配新的学生数据 */
        student_t *new_student = malloc(sizeof(student_t));
        new_student->id = 2000 + i;
        snprintf(new_student->name, sizeof(new_student->name), "新学生%d", i);
        new_student->score = 90.0f + i;
        
        printf("\n=== 更新 %d: 基地址从 %p 变为 %p ===\n", 
               i+1, old_student, new_student);
        
        /* 使用线程安全的方式更新基地址 */
        if (linx_hash_map_update_base_addr_safe("student", new_student) == 0) {
            printf("基地址更新成功\n");
            
            /* 释放旧内存 */
            if (old_student) {
                free(old_student);
            }
            old_student = new_student;
            g_student = new_student;
        } else {
            printf("基地址更新失败\n");
            free(new_student);
        }
    }
    
    printf("更新线程结束\n");
    g_stop = true;
    return NULL;
}

void setup_student_table(void)
{
    linx_hash_map_create_table("student", NULL);
    
    linx_hash_map_add_field("student", "id", offsetof(student_t, id), 
                           sizeof(int), FIELD_TYPE_INT32);
    linx_hash_map_add_field("student", "name", offsetof(student_t, name), 
                           sizeof(char) * 32, FIELD_TYPE_CHARBUF);
    linx_hash_map_add_field("student", "score", offsetof(student_t, score), 
                           sizeof(float), FIELD_TYPE_FLOAT);
}

void cleanup_demo(void)
{
    if (g_student) {
        free(g_student);
        g_student = NULL;
    }
}

int main(void)
{
    pthread_t reader1_tid, reader2_tid, updater_tid;
    int reader1_id = 1, reader2_id = 2;
    
    printf("=== 线程安全演示 ===\n");
    printf("说明：读取线程使用两种方式访问数据\n");
    printf("1. [安全]：使用锁保护的API\n");
    printf("2. [非安全]：直接使用非线程安全API\n\n");
    
    /* 初始化 */
    if (linx_hash_map_init() != 0) {
        fprintf(stderr, "初始化失败\n");
        return 1;
    }
    
    setup_student_table();
    
    /* 创建初始学生数据 */
    g_student = malloc(sizeof(student_t));
    g_student->id = 1001;
    strcpy(g_student->name, "初始学生");
    g_student->score = 85.5f;
    
    linx_hash_map_update_base_addr_safe("student", g_student);
    
    /* 创建线程 */
    pthread_create(&reader1_tid, NULL, reader_thread, &reader1_id);
    pthread_create(&reader2_tid, NULL, reader_thread, &reader2_id);
    pthread_create(&updater_tid, NULL, updater_thread, NULL);
    
    /* 等待线程结束 */
    pthread_join(updater_tid, NULL);
    g_stop = true;
    
    pthread_join(reader1_tid, NULL);
    pthread_join(reader2_tid, NULL);
    
    printf("\n=== 演示结束 ===\n");
    
    /* 清理 */
    cleanup_demo();
    linx_hash_map_deinit();
    
    return 0;
}