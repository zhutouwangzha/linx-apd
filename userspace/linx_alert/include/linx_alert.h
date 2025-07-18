#ifndef __LINX_ALERT_H__
#define __LINX_ALERT_H__ 

#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <stddef.h>

/* 前向声明 */
typedef struct linx_output_match_s linx_output_match_t;
typedef struct linx_thread_pool_s linx_thread_pool_t;

/* 输出类型枚举 */
typedef enum {
    LINX_OUTPUT_TYPE_STDOUT = 0,
    LINX_OUTPUT_TYPE_FILE,
    LINX_OUTPUT_TYPE_HTTP,
    LINX_OUTPUT_TYPE_SYSLOG,
    LINX_OUTPUT_TYPE_MAX
} linx_output_type_t;

/* 输出配置结构 */
typedef struct {
    linx_output_type_t type;
    bool enabled;
    union {
        struct {
            bool use_color;
            bool timestamp;
        } stdout_config;
        
        struct {
            char *file_path;
            size_t max_file_size;
            int max_files;
            bool append;
        } file_config;
        
        struct {
            char *url;
            char *headers;
            int timeout_ms;
        } http_config;
        
        struct {
            int facility;
            int priority;
        } syslog_config;
    } config;
} linx_output_config_t;

/* 告警消息结构 */
typedef struct {
    char *formatted_message;
    size_t message_len;
    linx_output_config_t *output_configs;
    int output_count;
    time_t timestamp;
    char *rule_name;
    int priority;
} linx_alert_message_t;

/* 告警系统结构 */
typedef struct {
    linx_thread_pool_t *thread_pool;
    linx_output_config_t *output_configs;
    int output_count;
    pthread_mutex_t config_mutex;
    bool initialized;
    
    /* 统计信息 */
    long total_alerts_sent;
    long total_alerts_failed;
    pthread_mutex_t stats_mutex;
} linx_alert_system_t;

/* 全局告警系统实例 */
extern linx_alert_system_t *g_alert_system;

/* 初始化和清理函数 */
int linx_alert_init(int thread_pool_size);
void linx_alert_cleanup(void);

/* 配置管理函数 */
int linx_alert_add_output_config(linx_output_config_t *config);
int linx_alert_remove_output_config(linx_output_type_t type);
int linx_alert_update_output_config(linx_output_type_t type, linx_output_config_t *config);

/* 核心输出函数 */
int linx_alert_send_async(linx_output_match_t *output_match, const char *rule_name, int priority);
int linx_alert_send_sync(linx_output_match_t *output_match, const char *rule_name, int priority);

/* 格式化和发送函数 */
int linx_alert_format_and_send(linx_output_match_t *output_match, const char *rule_name, int priority);

/* 统计信息函数 */
void linx_alert_get_stats(long *total_sent, long *total_failed);
void linx_alert_reset_stats(void);

/* 输出后端函数 */
int linx_alert_output_stdout(linx_alert_message_t *message, linx_output_config_t *config);
int linx_alert_output_file(linx_alert_message_t *message, linx_output_config_t *config);
int linx_alert_output_http(linx_alert_message_t *message, linx_output_config_t *config);
int linx_alert_output_syslog(linx_alert_message_t *message, linx_output_config_t *config);

/* 辅助函数 */
linx_alert_message_t *linx_alert_message_create(const char *formatted_message, const char *rule_name, int priority);
void linx_alert_message_destroy(linx_alert_message_t *message);

#endif /* __LINX_ALERT_H__ */
