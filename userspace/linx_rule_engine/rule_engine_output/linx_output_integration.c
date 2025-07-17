#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <time.h>

#include "linx_output_integration.h"

// 全局的事件数据实例（在实际应用中应该从事件队列获取）
static linx_event_data_t *g_current_event_data = NULL;

int linx_output_integration_init(void) {
    // 初始化全局事件数据
    g_current_event_data = linx_event_data_create();
    if (!g_current_event_data) {
        return -1;
    }
    
    return 0;
}

void linx_output_integration_cleanup(void) {
    if (g_current_event_data) {
        linx_event_data_destroy(g_current_event_data);
        g_current_event_data = NULL;
    }
}

// 自定义处理函数：获取当前用户名
static int handle_user_name(const void *data, char *output, size_t output_size) {
    (void)data; // 未使用的参数
    
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        snprintf(output, output_size, "%s", pw->pw_name);
    } else {
        snprintf(output, output_size, "unknown");
    }
    return 0;
}

// 自定义处理函数：获取当前进程名
static int handle_process_name(const void *data, char *output, size_t output_size) {
    (void)data; // 未使用的参数
    
    FILE *fp = fopen("/proc/self/comm", "r");
    if (fp) {
        if (fgets(output, output_size, fp)) {
            // 移除换行符
            char *newline = strchr(output, '\n');
            if (newline) *newline = '\0';
        }
        fclose(fp);
    } else {
        snprintf(output, output_size, "unknown");
    }
    return 0;
}

// 自定义处理函数：获取当前进程命令行
static int handle_process_cmdline(const void *data, char *output, size_t output_size) {
    (void)data; // 未使用的参数
    
    FILE *fp = fopen("/proc/self/cmdline", "r");
    if (fp) {
        size_t len = fread(output, 1, output_size - 1, fp);
        output[len] = '\0';
        
        // 将NULL字符替换为空格
        for (size_t i = 0; i < len; i++) {
            if (output[i] == '\0') {
                output[i] = ' ';
            }
        }
        fclose(fp);
    } else {
        snprintf(output, output_size, "unknown");
    }
    return 0;
}

int linx_output_integration_register_default_bindings(linx_output_match_t *output_match) {
    if (!output_match) {
        return -1;
    }
    
    // 注册基本字段绑定
    struct {
        const char *variable_name;
        field_type_t field_type;
        size_t offset;
        size_t size;
        int (*custom_handler)(const void *data, char *output, size_t output_size);
    } bindings[] = {
        // 时间字段
        {"%evt.time", FIELD_TYPE_TIMESTAMP, LINX_EVENT_FIELD_OFFSET(timestamp), LINX_EVENT_FIELD_SIZE(timestamp), NULL},
        
        // 进程字段
        {"%proc.pid", FIELD_TYPE_INT32, LINX_EVENT_FIELD_OFFSET(pid), LINX_EVENT_FIELD_SIZE(pid), NULL},
        {"%proc.ppid", FIELD_TYPE_INT32, LINX_EVENT_FIELD_OFFSET(ppid), LINX_EVENT_FIELD_SIZE(ppid), NULL},
        {"%proc.name", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(process_name), LINX_EVENT_FIELD_SIZE(process_name), NULL},
        {"%proc.cmdline", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(cmdline), LINX_EVENT_FIELD_SIZE(cmdline), NULL},
        
        // 用户字段
        {"%user.name", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(username), LINX_EVENT_FIELD_SIZE(username), NULL},
        {"%user.uid", FIELD_TYPE_UINT32, LINX_EVENT_FIELD_OFFSET(uid), LINX_EVENT_FIELD_SIZE(uid), NULL},
        
        // 文件字段
        {"%fd.name", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(file_name), LINX_EVENT_FIELD_SIZE(file_name), NULL},
        {"%fd.path", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(file_path), LINX_EVENT_FIELD_SIZE(file_path), NULL},
        {"%fd.type", FIELD_TYPE_UINT32, LINX_EVENT_FIELD_OFFSET(file_type), LINX_EVENT_FIELD_SIZE(file_type), NULL},
        
        // 事件字段
        {"%evt.type", FIELD_TYPE_UINT32, LINX_EVENT_FIELD_OFFSET(event_type), LINX_EVENT_FIELD_SIZE(event_type), NULL},
        {"%evt.dir", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(event_direction), LINX_EVENT_FIELD_SIZE(event_direction), NULL},
        {"%evt.arg.data", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(event_data), LINX_EVENT_FIELD_SIZE(event_data), NULL},
        {"%evt.res", FIELD_TYPE_INT32, LINX_EVENT_FIELD_OFFSET(event_result), LINX_EVENT_FIELD_SIZE(event_result), NULL},
        
        // 网络字段
        {"%net.src_ip", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(src_ip), LINX_EVENT_FIELD_SIZE(src_ip), NULL},
        {"%net.dst_ip", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(dst_ip), LINX_EVENT_FIELD_SIZE(dst_ip), NULL},
        {"%net.src_port", FIELD_TYPE_UINT16, LINX_EVENT_FIELD_OFFSET(src_port), LINX_EVENT_FIELD_SIZE(src_port), NULL},
        {"%net.dst_port", FIELD_TYPE_UINT16, LINX_EVENT_FIELD_OFFSET(dst_port), LINX_EVENT_FIELD_SIZE(dst_port), NULL},
        {"%net.proto", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(protocol), LINX_EVENT_FIELD_SIZE(protocol), NULL},
        
        // 容器字段
        {"%container.id", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(container_id), LINX_EVENT_FIELD_SIZE(container_id), NULL},
        {"%container.name", FIELD_TYPE_STRING, LINX_EVENT_FIELD_OFFSET(container_name), LINX_EVENT_FIELD_SIZE(container_name), NULL},
    };
    
    // 注册所有基本绑定
    for (size_t i = 0; i < sizeof(bindings) / sizeof(bindings[0]); i++) {
        if (bindings[i].custom_handler) {
            linx_output_match_add_custom_binding(output_match, bindings[i].variable_name, bindings[i].custom_handler);
        } else {
            linx_output_match_add_binding(output_match, bindings[i].variable_name, 
                                         bindings[i].field_type, bindings[i].offset, bindings[i].size);
        }
    }
    
    // 注册自定义处理函数（用于获取当前系统信息）
    linx_output_match_add_custom_binding(output_match, "%user.name.current", handle_user_name);
    linx_output_match_add_custom_binding(output_match, "%proc.name.current", handle_process_name);
    linx_output_match_add_custom_binding(output_match, "%proc.cmdline.current", handle_process_cmdline);
    
    return 0;
}

int linx_output_integration_create_match_for_rule(const linx_rule_t *rule, 
                                                  linx_output_match_t **output_match) {
    if (!rule || !output_match || !rule->output) {
        return -1;
    }
    
    // 创建输出匹配结构
    *output_match = linx_output_match_create();
    if (!*output_match) {
        return -1;
    }
    
    // 注册默认字段绑定
    if (linx_output_integration_register_default_bindings(*output_match) != 0) {
        linx_output_match_destroy(*output_match);
        *output_match = NULL;
        return -1;
    }
    
    // 编译输出模板
    if (linx_output_match_compile(*output_match, rule->output) != 0) {
        linx_output_match_destroy(*output_match);
        *output_match = NULL;
        return -1;
    }
    
    return 0;
}

int linx_output_integration_format_and_print(const linx_rule_t *rule,
                                             const linx_event_data_t *event_data) {
    if (!rule || !event_data) {
        return -1;
    }
    
    // 创建输出匹配
    linx_output_match_t *output_match = NULL;
    if (linx_output_integration_create_match_for_rule(rule, &output_match) != 0) {
        return -1;
    }
    
    // 格式化并打印输出
    int ret = linx_output_match_format_and_print(output_match, event_data);
    
    // 清理资源
    linx_output_match_destroy(output_match);
    
    return ret;
}

int linx_output_integration_create_matches_for_rules(const linx_rule_t *rules,
                                                     size_t rule_count,
                                                     linx_output_match_t **output_matches) {
    if (!rules || !output_matches) {
        return -1;
    }
    
    int success_count = 0;
    
    for (size_t i = 0; i < rule_count; i++) {
        if (linx_output_integration_create_match_for_rule(&rules[i], &output_matches[i]) == 0) {
            success_count++;
        } else {
            output_matches[i] = NULL;
        }
    }
    
    return success_count;
}

// 事件数据管理函数的实现
linx_event_data_t *linx_event_data_create(void) {
    linx_event_data_t *event_data = calloc(1, sizeof(linx_event_data_t));
    if (!event_data) {
        return NULL;
    }
    
    // 设置默认值
    event_data->pid = getpid();
    event_data->ppid = getppid();
    event_data->uid = getuid();
    
    // 设置当前时间
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    event_data->timestamp.tv_sec = ts.tv_sec;
    event_data->timestamp.tv_nsec = ts.tv_nsec;
    
    return event_data;
}

void linx_event_data_destroy(linx_event_data_t *event_data) {
    if (!event_data) {
        return;
    }
    
    // 释放字符串字段
    free(event_data->process_name);
    free(event_data->cmdline);
    free(event_data->username);
    free(event_data->file_name);
    free(event_data->file_path);
    free(event_data->event_direction);
    free(event_data->event_data);
    free(event_data->src_ip);
    free(event_data->dst_ip);
    free(event_data->protocol);
    free(event_data->container_id);
    free(event_data->container_name);
    
    free(event_data);
}

int linx_event_data_set_string(linx_event_data_t *event_data, const char *field_name, const char *value) {
    if (!event_data || !field_name) {
        return -1;
    }
    
    char **field_ptr = NULL;
    
    // 查找对应的字段
    if (strcmp(field_name, "process_name") == 0) {
        field_ptr = &event_data->process_name;
    } else if (strcmp(field_name, "cmdline") == 0) {
        field_ptr = &event_data->cmdline;
    } else if (strcmp(field_name, "username") == 0) {
        field_ptr = &event_data->username;
    } else if (strcmp(field_name, "file_name") == 0) {
        field_ptr = &event_data->file_name;
    } else if (strcmp(field_name, "file_path") == 0) {
        field_ptr = &event_data->file_path;
    } else if (strcmp(field_name, "event_direction") == 0) {
        field_ptr = &event_data->event_direction;
    } else if (strcmp(field_name, "event_data") == 0) {
        field_ptr = &event_data->event_data;
    } else if (strcmp(field_name, "src_ip") == 0) {
        field_ptr = &event_data->src_ip;
    } else if (strcmp(field_name, "dst_ip") == 0) {
        field_ptr = &event_data->dst_ip;
    } else if (strcmp(field_name, "protocol") == 0) {
        field_ptr = &event_data->protocol;
    } else if (strcmp(field_name, "container_id") == 0) {
        field_ptr = &event_data->container_id;
    } else if (strcmp(field_name, "container_name") == 0) {
        field_ptr = &event_data->container_name;
    } else {
        return -1; // 未知字段
    }
    
    // 释放旧值并设置新值
    free(*field_ptr);
    *field_ptr = value ? strdup(value) : NULL;
    
    return 0;
}

int linx_event_data_set_int(linx_event_data_t *event_data, const char *field_name, int64_t value) {
    if (!event_data || !field_name) {
        return -1;
    }
    
    if (strcmp(field_name, "pid") == 0) {
        event_data->pid = (pid_t)value;
    } else if (strcmp(field_name, "ppid") == 0) {
        event_data->ppid = (pid_t)value;
    } else if (strcmp(field_name, "uid") == 0) {
        event_data->uid = (uid_t)value;
    } else if (strcmp(field_name, "file_type") == 0) {
        event_data->file_type = (uint32_t)value;
    } else if (strcmp(field_name, "event_type") == 0) {
        event_data->event_type = (uint32_t)value;
    } else if (strcmp(field_name, "event_result") == 0) {
        event_data->event_result = (int32_t)value;
    } else if (strcmp(field_name, "src_port") == 0) {
        event_data->src_port = (uint16_t)value;
    } else if (strcmp(field_name, "dst_port") == 0) {
        event_data->dst_port = (uint16_t)value;
    } else {
        return -1; // 未知字段
    }
    
    return 0;
}

int linx_event_data_set_current_time(linx_event_data_t *event_data) {
    if (!event_data) {
        return -1;
    }
    
    struct timespec ts;
    int ret = clock_gettime(CLOCK_REALTIME, &ts);
    if (ret == 0) {
        event_data->timestamp.tv_sec = ts.tv_sec;
        event_data->timestamp.tv_nsec = ts.tv_nsec;
    }
    return ret;
}