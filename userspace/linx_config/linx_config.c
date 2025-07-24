#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linx_log.h"
#include "linx_config.h"
#include "linx_yaml.h"

linx_global_config_t *linx_global_config = NULL;

int linx_config_init(void)
{
    if (linx_global_config) {
        return 0;
    }

    linx_global_config = (linx_global_config_t *)malloc(sizeof(linx_global_config_t));
    if (!linx_global_config) {
        return -1;
    }

    bzero(linx_global_config, sizeof(linx_global_config_t));

    return 0;
}

void linx_config_deinit(void)
{
    if (!linx_global_config) {
        return;
    }

    if (linx_global_config->engine) {
        free(linx_global_config->engine);
        linx_global_config->engine = NULL;
    }

    if (linx_global_config->log_config.log_level) {
        free(linx_global_config->log_config.log_level);
        linx_global_config->log_config.log_level = NULL;
    }

    if (linx_global_config->log_config.output) {
        free(linx_global_config->log_config.output);
        linx_global_config->log_config.output = NULL;
    }

    free(linx_global_config);
    linx_global_config = NULL;
}

linx_global_config_t *linx_config_get(void)
{
    return linx_global_config;
}

int linx_config_load(const char *config_file)
{
    int ret = 0;
    linx_yaml_node_t *root;
    linx_global_config_t *config = linx_config_get();

    /**
     * linx_apd.yaml 文件也应该分优先级，例如：
     * 1. 环境变量
     * 2. 命令行参数
     * 3. 默认安装路径
     * 4. 代码源码路径
    */
    root = linx_yaml_load(config_file);
    if (!root) {
        return -1;
    }

    config->engine = strdup(linx_yaml_get_string(root, "engine.kind", "unknown"));
    config->log_config.output = strdup(linx_yaml_get_string(root, "log.output", "stderr"));
    config->log_config.log_level = strdup(linx_yaml_get_string(root, "log.level", "ERROR"));

    linx_yaml_node_free(root);
    return ret;
}
