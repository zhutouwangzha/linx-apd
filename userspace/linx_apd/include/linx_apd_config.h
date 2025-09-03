#ifndef __LINX_APD_CONFIG_H__
#define __LINX_APD_CONFIG_H__

#include <stdbool.h>

/* 多线程规则匹配配置 */
typedef struct {
    bool enable_mt_match;        /* 是否启用多线程规则匹配 */
    int num_match_threads;       /* 匹配线程数量 */
    int match_queue_size;        /* 匹配任务队列大小 */
} linx_mt_config_t;

/* APD内部配置结构 */
typedef struct {
    linx_mt_config_t mt_config;  /* 多线程配置 */
} linx_apd_config_t;

/* 获取APD内部配置 */
linx_apd_config_t *linx_apd_config_get(void);

/* 初始化APD内部配置 */
int linx_apd_config_init(void);

/* 设置多线程匹配配置 */
void linx_apd_config_set_mt_match(bool enable, int num_threads);

/* 清理APD内部配置 */
void linx_apd_config_deinit(void);

#endif /* __LINX_APD_CONFIG_H__ */