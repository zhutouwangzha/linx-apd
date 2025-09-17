#ifndef __LINX_THREAD_BASE_ADDR_H__
#define __LINX_THREAD_BASE_ADDR_H__

#include <pthread.h>
#include <stdbool.h>
#include "uthash_ext.h"

/**
 * 线程特定的base_addr映射表
 * 每个线程只需要维护table_name -> base_addr的映射
 */
typedef struct {
    char *table_name;           /* 表名（键） */
    void *base_addr;           /* 该线程中该表的基地址 */
    UT_hash_handle hh;
} thread_base_addr_entry_t;

/**
 * 线程特定的base_addr存储
 */
typedef struct {
    pthread_t thread_id;                    /* 线程ID */
    thread_base_addr_entry_t *base_addrs;  /* base_addr映射表 */
    bool initialized;                       /* 初始化标志 */
} thread_base_addr_context_t;

/**
 * 初始化线程特定base_addr系统
 * @return 0 成功，-1 失败
 */
int linx_thread_base_addr_init(void);

/**
 * 清理线程特定base_addr系统
 */
void linx_thread_base_addr_deinit(void);

/**
 * 为当前线程创建base_addr上下文
 * @return 0 成功，-1 失败
 */
int linx_thread_base_addr_create(void);

/**
 * 销毁当前线程的base_addr上下文
 */
void linx_thread_base_addr_destroy(void);

/**
 * 设置当前线程中指定表的base_addr
 * @param table_name 表名
 * @param base_addr 基地址
 * @return 0 成功，-1 失败
 */
int linx_thread_base_addr_set(const char *table_name, void *base_addr);

/**
 * 获取当前线程中指定表的base_addr
 * @param table_name 表名
 * @return base_addr，NULL表示未找到
 */
void *linx_thread_base_addr_get(const char *table_name);

/**
 * 批量设置当前线程的base_addr
 * @param tables base_addr更新表数组
 * @param num_tables 数组大小
 * @return 0 成功，失败则返回失败的索引
 */
int linx_thread_base_addr_set_batch(field_update_table_t *tables, size_t num_tables);

#endif /* __LINX_THREAD_BASE_ADDR_H__ */