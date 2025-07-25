#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cJSON.h"

#include "linx_log.h"
#include "linx_config.h"
#include "linx_yaml.h"
#include "linx_syscall_table.h"

linx_global_config_t *linx_global_config = NULL;

static int linx_config_fill_interest_syscall_table(char *file_path)
{
    cJSON *json_config, *member, *interest;
    char *jsonstr;
    struct stat file_stat;
    off_t byte_read;
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        LINX_LOG_ERROR("open %s failed", file_path);
        return -1;
    }

    if (fstat(fd, &file_stat) == -1) {
        LINX_LOG_ERROR("fstat %s failed", file_path);
        close(fd);
        return -1;
    }

    jsonstr = malloc(file_stat.st_size);
    if (!jsonstr) {
        LINX_LOG_ERROR("malloc failed");
        close(fd);
        return -1;
    }

    byte_read = read(fd, jsonstr, file_stat.st_size);
    if (byte_read != file_stat.st_size) {
        LINX_LOG_ERROR("read %s failed", file_path);
        free(jsonstr);
        close(fd);
        return -1;
    }

    close(fd);

    json_config = cJSON_Parse(jsonstr);
    if (!json_config) {
        LINX_LOG_ERROR("parse %s failed", file_path);
        free(jsonstr);
        return -1;
    }

    free(jsonstr);

    for (int i = 0; i < LINX_SYSCALL_MAX_IDX; i++) {
        member = cJSON_GetObjectItem(json_config, g_linx_syscall_table[i].name);
        if (!member) {
            continue;
        }

        interest = cJSON_GetObjectItem(member, "interesting");
        if (!interest) {
            continue;
        }

        linx_global_config->engine.data.ebpf.interest_syscall_table[i] = 
            interest->valueint;
    }

    cJSON_Delete(json_config);

    return 0;
}

static int linx_config_fill_engine(linx_yaml_node_t *root)
{
    char node_path[256];
    int count;

    linx_global_config->engine.kind = strdup(linx_yaml_get_string(root, "engine.kind", "unknown"));
    if (!linx_global_config->engine.kind) {
        return -1;
    }

    if (strcmp(linx_global_config->engine.kind, "ebpf") == 0) {
        linx_global_config->engine.data.ebpf.drop_mode = 
            linx_yaml_get_bool(root, "engine.ebpf.drop_mode", 0);

        linx_global_config->engine.data.ebpf.drop_failed = 
            linx_yaml_get_bool(root, "engine.ebpf.drop_failed", 0);
        
        count = linx_yaml_get_sequence_length(root, "engine.ebpf.filter_pids");
        for (int i = 0; i < count; i++) {
            snprintf(node_path, sizeof(node_path), "engine.ebpf.filter_pids.%d", i);
            linx_global_config->engine.data.ebpf.filter_pids[i] = 
                linx_yaml_get_int(root, node_path, 0);
        }

        count = linx_yaml_get_sequence_length(root, "engine.ebpf.filter_comms");
        for (int i = 0; i < count; i++) {
            snprintf(node_path, sizeof(node_path), "engine.ebpf.filter_comms.%d", i);
            snprintf((char *)linx_global_config->engine.data.ebpf.filter_comms[i], 
                     LINX_COMM_MAX_SIZE, "%s", linx_yaml_get_string(root, node_path, ""));
        }

        linx_config_fill_interest_syscall_table((char *)linx_yaml_get_string(root, "engine.ebpf.interest_syscall_file", NULL));

    } else if (strcmp(linx_global_config->engine.kind, "kmod") == 0) {

    } else {
        return -1;
    }

    return 0;
}

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

    ret = linx_config_fill_engine(root);
    if (ret) {
        return ret;
    }

    
    linx_global_config->log_config.output = strdup(linx_yaml_get_string(root, "log.output", "stderr"));
    linx_global_config->log_config.log_level = strdup(linx_yaml_get_string(root, "log.level", "ERROR"));

    linx_yaml_node_free(root);
    return ret;
}
