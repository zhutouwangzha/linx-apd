#include "linx_alert.h"
#include <stdint.h>

static linx_alert_t *s_alert = NULL;

typedef struct {
    linx_alert_message_t *message;
} linx_alert_task_arg_t;

static int linx_alert_send_to_outputs(linx_alert_message_t *message)
{
    int ret;
    int success_count = 0, total_count = 0;
    linx_alert_config_t *config;

    if (!message || !message->message) {
        return -1;
    }

    for (int i = 0; i < LINX_ALERT_TYPE_MAX; i++) {
        config = &message->config[i];

        if (!config->enabled) {
            continue;
        }

        switch (config->type) {
        case LINX_ALERT_TYPE_STDOUT:
            ret = linx_alert_output_stdout(message, config);
            break;
        case LINX_ALERT_TYPE_FILE:
            ret = linx_alert_output_file(message, config);
            break;
        case LINX_ALERT_TYPE_HTTP:
            ret = linx_alert_output_http(message, config);
            break;
        case LINX_ALERT_TYPE_SYSLOG:
            ret = linx_alert_output_syslog(message, config);
            break;
        default:
            continue;
        }

        total_count++;

        if (ret == 0) {
            success_count++;
        }
    }

    pthread_mutex_lock(&s_alert->stats_mutex);
    if (success_count > 0) {
        s_alert->total_alerts_send++;
    }
    
    if (success_count < total_count) {
        s_alert->total_alerts_failed++;
    }
    pthread_mutex_unlock(&s_alert->stats_mutex);

    return (success_count > 0) ? 0 : -1;
}

static void *linx_alert_worker_task(void *arg, int *should_stop)
{
    (void)should_stop;
    linx_alert_task_arg_t *task_arg = (linx_alert_task_arg_t *)arg;

    /* 检查参数有效性 */
    if (!task_arg) {
        return NULL;
    }

    /* 检查指针是否有效 */
    if ((uintptr_t)task_arg < 0x1000 || (uintptr_t)task_arg > 0x7fffffffffff) {
        return NULL;
    }

    if (task_arg->message) {
        /* 检查message指针是否有效 */
        if ((uintptr_t)task_arg->message >= 0x1000 && (uintptr_t)task_arg->message <= 0x7fffffffffff) {
            linx_alert_send_to_outputs(task_arg->message);
            linx_alert_message_destroy(task_arg->message);
        }
    }

    free(task_arg);
    return NULL;
}

int linx_alert_init(int thread_pool_size)
{
    if (s_alert != NULL) {
        return 0;
    }

    s_alert = malloc(sizeof(linx_alert_t));
    if (!s_alert) {
        return -1;
    }

    memset(s_alert, 0, sizeof(linx_alert_t));

    if (thread_pool_size <= 0) {
        thread_pool_size = 4;
    }

    s_alert->thread_pool = linx_thread_pool_create(thread_pool_size);
    if (!s_alert->thread_pool) {
        free(s_alert);
        s_alert = NULL;
        return -1;
    }

    if (pthread_mutex_init(&s_alert->config_mutex, NULL) != 0) {
        linx_thread_pool_destroy(s_alert->thread_pool, 1);
        free(s_alert);
        s_alert = NULL;
        return -1;
    }

    if (pthread_mutex_init(&s_alert->stats_mutex, NULL) != 0) {
        pthread_mutex_destroy(&s_alert->config_mutex);
        linx_thread_pool_destroy(s_alert->thread_pool, 1);
        free(s_alert);
        s_alert = NULL;
        return -1;
    }

    s_alert->initialized = true;
    s_alert->total_alerts_send = 0;
    s_alert->total_alerts_failed = 0;

    /* 现在处于测试阶段，在这里先设置好config，实际应该从yaml文件导入 */
    s_alert->config[LINX_ALERT_TYPE_STDOUT].type = LINX_ALERT_TYPE_STDOUT;
    s_alert->config[LINX_ALERT_TYPE_STDOUT].enabled = true;

    return 0;
}
void linx_alert_deinit(void)
{
    if (s_alert == NULL) {
        return;
    }

    s_alert->initialized = false;

    if (s_alert->thread_pool != NULL) {
        linx_thread_pool_destroy(s_alert->thread_pool, 1);
        s_alert->thread_pool = NULL;
    }

    pthread_mutex_lock(&s_alert->config_mutex);
    for (int i = 0; i < LINX_ALERT_TYPE_MAX; i++) {
        if (s_alert->config[i].enabled) {
            s_alert->config[i].enabled = false;
        }
    }
    pthread_mutex_unlock(&s_alert->config_mutex);

    pthread_mutex_destroy(&s_alert->config_mutex);
    pthread_mutex_destroy(&s_alert->stats_mutex);

    free(s_alert);
    s_alert = NULL;
}

/* 配置管理函数 */
int linx_alert_set_config_enable(linx_alert_type_t type, bool enable)
{
    if (!s_alert || type < 0 || type >= LINX_ALERT_TYPE_MAX) {
        return -1;
    }

    s_alert->config[type].enabled = enable;

    return 0;
}

int linx_alert_update_config(linx_alert_config_t config)
{
    if (!s_alert || config.type < 0 || config.type >= LINX_ALERT_TYPE_MAX) {
        return -1;
    }

    memcpy(&s_alert->config[config.type], &config, sizeof(linx_alert_config_t));

    return 0;
}


/* 核心输出函数 */
int linx_alert_send_async(linx_output_match_t *output, const char *rule_name, int priority)
{
    char buffer[4096];
    int ret;
    linx_alert_message_t *message;
    linx_alert_task_arg_t *task_arg;

    if (s_alert == NULL || !s_alert->initialized || output == NULL) {
        return -1;
    }

    ret = linx_output_match_format(output, buffer, sizeof(buffer));
    if (ret < 0) {
        return -1;
    }

    message = linx_alert_message_create(buffer, rule_name, priority);
    if (!message) {
        return -1;
    }

    pthread_mutex_lock(&s_alert->config_mutex);
    memcpy(message->config, s_alert->config, sizeof(s_alert->config));
    pthread_mutex_unlock(&s_alert->config_mutex);

    task_arg = malloc(sizeof(linx_alert_task_arg_t));
    if (!task_arg) {
        linx_alert_message_destroy(message);
        return -1;
    }

    task_arg->message = message;

    ret = linx_thread_pool_add_task(s_alert->thread_pool, linx_alert_worker_task, task_arg);
    if (ret) {
        free(task_arg);
        linx_alert_message_destroy(message);
        return -1;
    }

    return 0;
}

int linx_alert_send_sync(linx_output_match_t *output, const char *rule_name, int priority)
{
    int ret;
    char buffer[4096];
    linx_alert_message_t *message;

    if (!s_alert || !s_alert->initialized || !output) {
        return -1;
    }

    ret = linx_output_match_format(output, buffer, sizeof(buffer));
    if (ret) {
        return -1;
    }

    message = linx_alert_message_create(buffer, rule_name, priority);
    if (!message) {
        return -1;
    }

    pthread_mutex_lock(&s_alert->config_mutex);
    memcpy(message->config, s_alert->config, sizeof(s_alert->config));
    pthread_mutex_unlock(&s_alert->config_mutex);

    /* 直接发送 */
    ret = linx_alert_send_to_outputs(message);

    linx_alert_message_destroy(message);
    return 0;
}

/* 格式化和发送函数 */
int linx_alert_format_and_send(linx_output_match_t *output, const char *rule_name, int priority)
{
    return linx_alert_send_sync(output, rule_name, priority);
}

/* 统计信息函数 */
void linx_alert_get_stats(long *total_send, long *total_fail)
{
    if (!s_alert) {
        if (total_send) {
            *total_send = 0;
        }

        if (total_fail) {
            *total_fail = 0;
        }

        return;
    }

    pthread_mutex_lock(&s_alert->stats_mutex);
    if (total_send) {
        *total_send = s_alert->total_alerts_send;
    }
    
    if (total_fail) {
        *total_fail = s_alert->total_alerts_failed;
    }
    pthread_mutex_unlock(&s_alert->stats_mutex);
}

/* 辅助函数 */
linx_alert_message_t *linx_alert_message_create(const char *formatted_message, const char *rule_name, int priority)
{
    linx_alert_message_t *message;

    if (formatted_message == NULL) {
        return NULL;
    }

    message = malloc(sizeof(linx_alert_message_t));
    if (!message) {
        return NULL;
    }

    memset(message, 0, sizeof(linx_alert_message_t));

    message->message_len = strlen(formatted_message);
    message->message = strdup(formatted_message);

    if (rule_name) {
        message->rule_name = strdup(rule_name);
    }

    message->priority = priority;
    
    return message;
}

void linx_alert_message_destroy(linx_alert_message_t *message)
{
    if (!message) {
        return;
    }

    /* 检查指针是否有效，避免访问无效内存 */
    if ((uintptr_t)message < 0x1000 || (uintptr_t)message > 0x7fffffffffff) {
        /* 无效指针，可能已被损坏 */
        return;
    }

    if (message->message) {
        free(message->message);
        message->message = NULL;
    }

    if (message->rule_name) {
        free(message->rule_name);
        message->rule_name = NULL;
    }

    free(message);
}
