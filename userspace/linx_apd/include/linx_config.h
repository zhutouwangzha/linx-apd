#ifndef __LINX_CONFIG_H__
#define __LINX_CONFIG_H__

#include <stdbool.h>

/* 多线程规则匹配配置 */
typedef struct {
    bool enable_mt_match;        /* 是否启用多线程规则匹配 */
    int num_match_threads;       /* 匹配线程数量 */
    int match_queue_size;        /* 匹配任务队列大小 */
} linx_mt_config_t;

/* 全局配置结构 */
typedef struct {
    linx_mt_config_t mt_config;  /* 多线程配置 */
} linx_config_t;

/* 获取全局配置 */
linx_config_t *linx_get_config(void);

/* 初始化配置 */
int linx_config_init(void);

/* 清理配置 */
void linx_config_deinit(void);

#endif /* __LINX_CONFIG_H__ */