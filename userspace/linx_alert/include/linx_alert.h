#ifndef __LINX_ALERT_H__
#define __LINX_ALERT_H__ 

#include <stdbool.h>
#include <pthread.h>
#include <time.h>

#include "linx_thread_pool.h"
#include "linx_rule_engine_match.h"

typedef enum {
    LINX_ALERT_TYPE_STDOUT,
    LINX_ALERT_TYPE_FILE,
    LINX_ALERT_TYPE_HTTP,
    LINX_ALERT_TYPE_SYSLOG,
    LINX_ALERT_TYPE_MAX
} linx_alert_type_t;

typedef struct {
    linx_alert_type_t type;
    bool enabled;
    union {
        struct {
            bool use_color;
        } stdout_config;

        struct {
            char *file_path;
        } file_config;

        struct {
            char *url;
            char *headers;
        } http_config;

        struct {
            int facility;
        } syslog_config;
    } config;
} linx_alert_config_t;

typedef struct {
    char *message;
    size_t message_len;
    linx_alert_config_t config[LINX_ALERT_TYPE_MAX];
    char *rule_name;
    int priority;
} linx_alert_message_t;

typedef struct {
    linx_thread_pool_t *thread_pool;
    linx_alert_config_t config[LINX_ALERT_TYPE_MAX];
    pthread_mutex_t config_mutex;
    bool initialized;

    long total_alerts_send;
    long total_alerts_failed;
    pthread_mutex_t stats_mutex;
} linx_alert_t;

/* 初始化和清理函数 */
int linx_alert_init(int thread_pool_size);
void linx_alert_deinit(void);

/* 配置管理函数 */
int linx_alert_set_config_enable(linx_alert_type_t type, bool enable);
int linx_alert_update_config(linx_alert_config_t config);

/* 核心输出函数 */
int linx_alert_send_async(linx_output_match_t *output, const char *rule_name, int priority);
int linx_alert_send_sync(linx_output_match_t *output, const char *rule_name, int priority);

/* 格式化和发送函数 */
int linx_alert_format_and_send(linx_output_match_t *output, const char *rule_name, int priority);

/* 统计信息函数 */
void linx_alert_get_stats(long *total_send, long *total_fail);

/* 输出后端函数 */
int linx_alert_output_stdout(linx_alert_message_t *message, linx_alert_config_t *config);
int linx_alert_output_file(linx_alert_message_t *message, linx_alert_config_t *config);
int linx_alert_output_http(linx_alert_message_t *message, linx_alert_config_t *config);
int linx_alert_output_syslog(linx_alert_message_t *message, linx_alert_config_t *config);

/* 辅助函数 */
linx_alert_message_t *linx_alert_message_create(const char *message, const char *rule_name, int priority);
void linx_alert_message_destroy(linx_alert_message_t *message);

#endif /* __LINX_ALERT_H__ */
