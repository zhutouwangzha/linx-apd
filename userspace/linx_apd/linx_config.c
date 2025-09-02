#include <stdlib.h>
#include <unistd.h>
#include "linx_config.h"

static linx_config_t *g_config = NULL;

/* 获取全局配置 */
linx_config_t *linx_get_config(void)
{
    return g_config;
}

/* 初始化配置 */
int linx_config_init(void)
{
    if (g_config) {
        return 0;
    }
    
    g_config = calloc(1, sizeof(linx_config_t));
    if (!g_config) {
        return -1;
    }
    
    /* 设置默认配置 */
    g_config->mt_config.enable_mt_match = false;  /* 默认禁用多线程匹配 */
    g_config->mt_config.num_match_threads = sysconf(_SC_NPROCESSORS_ONLN); /* 默认使用CPU核心数 */
    g_config->mt_config.match_queue_size = 1024;  /* 默认队列大小 */
    
    /* TODO: 从配置文件读取配置 */
    
    return 0;
}

/* 清理配置 */
void linx_config_deinit(void)
{
    if (g_config) {
        free(g_config);
        g_config = NULL;
    }
}