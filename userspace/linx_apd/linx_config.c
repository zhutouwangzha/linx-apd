#include <stdlib.h>
#include <unistd.h>
#include "linx_apd_config.h"

static linx_apd_config_t *g_config = NULL;

/* 获取APD内部配置 */
linx_apd_config_t *linx_apd_config_get(void)
{
    return g_config;
}

/* 初始化APD内部配置 */
int linx_apd_config_init(void)
{
    if (g_config) {
        return 0;
    }
    
    g_config = calloc(1, sizeof(linx_apd_config_t));
    if (!g_config) {
        return -1;
    }
    
    /* 设置默认配置 */
    g_config->mt_config.enable_mt_match = true;   /* 默认启用多线程匹配 */
    g_config->mt_config.num_match_threads = sysconf(_SC_NPROCESSORS_ONLN); /* 默认使用CPU核心数 */
    g_config->mt_config.match_queue_size = 1024;  /* 默认队列大小 */
    
    /* TODO: 从配置文件读取配置 */
    
    return 0;
}

/* 设置多线程匹配配置 */
void linx_apd_config_set_mt_match(bool enable, int num_threads)
{
    if (g_config) {
        g_config->mt_config.enable_mt_match = enable;
        if (num_threads > 0) {
            g_config->mt_config.num_match_threads = num_threads;
        }
    }
}

/* 清理APD内部配置 */
void linx_apd_config_deinit(void)
{
    if (g_config) {
        free(g_config);
        g_config = NULL;
    }
}