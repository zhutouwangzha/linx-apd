#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "linx_alert.h"
#include "output_match_func.h"

/**
 * @brief 创建测试用的输出匹配对象
 * @return 输出匹配对象，失败返回NULL
 */
static linx_output_match_t *create_test_output_match(void)
{
    linx_output_match_t *output_match = NULL;
    
    // 创建一个简单的输出格式：时间戳 + 进程信息 + 自定义消息
    char *format_string = "Alert: Process %proc.name (PID: %proc.pid) triggered rule at %evt.time - %custom.message";
    
    if (linx_output_match_compile(&output_match, format_string) != 0) {
        printf("Failed to compile output format\n");
        return NULL;
    }
    
    return output_match;
}

/**
 * @brief 设置测试用的字段表
 */
static void setup_test_fields(void)
{
    // 这里假设您已经有了字段表系统
    // 实际使用时，您需要根据事件数据填充字段表
    
    // 示例：设置一些测试字段
    // linx_hash_map_create_table("proc", some_process_data);
    // linx_hash_map_add_field("proc", "name", offset_of_name, size_of_name, FIELD_TYPE_STRING);
    // linx_hash_map_add_field("proc", "pid", offset_of_pid, size_of_pid, FIELD_TYPE_INT);
    
    printf("Note: Field table setup is required for variable substitution\n");
}

/**
 * @brief 配置多种输出方式
 */
static int configure_outputs(void)
{
    int result = 0;
    
    // 配置 stdout 输出
    linx_output_config_t stdout_config = {0};
    stdout_config.type = LINX_OUTPUT_TYPE_STDOUT;
    stdout_config.enabled = true;
    stdout_config.config.stdout_config.use_color = true;
    stdout_config.config.stdout_config.timestamp = true;
    
    if (linx_alert_add_output_config(&stdout_config) != 0) {
        printf("Failed to add stdout output config\n");
        result = -1;
    }
    
    // 配置文件输出
    linx_output_config_t file_config = {0};
    file_config.type = LINX_OUTPUT_TYPE_FILE;
    file_config.enabled = true;
    file_config.config.file_config.file_path = strdup("/tmp/linx_alerts.log");
    file_config.config.file_config.max_file_size = 10 * 1024 * 1024;  // 10MB
    file_config.config.file_config.max_files = 5;
    file_config.config.file_config.append = true;
    
    if (linx_alert_add_output_config(&file_config) != 0) {
        printf("Failed to add file output config\n");
        result = -1;
    }
    
    // 配置 HTTP 输出（可选，需要有测试服务器）
    /*
    linx_output_config_t http_config = {0};
    http_config.type = LINX_OUTPUT_TYPE_HTTP;
    http_config.enabled = true;
    http_config.config.http_config.url = strdup("http://localhost:8080/alerts");
    http_config.config.http_config.timeout_ms = 5000;
    http_config.config.http_config.headers = strdup("Authorization: Bearer test-token");
    
    if (linx_alert_add_output_config(&http_config) != 0) {
        printf("Failed to add HTTP output config\n");
    }
    */
    
    // 配置 syslog 输出
    linx_output_config_t syslog_config = {0};
    syslog_config.type = LINX_OUTPUT_TYPE_SYSLOG;
    syslog_config.enabled = true;
    syslog_config.config.syslog_config.facility = 16;  // LOG_LOCAL0
    syslog_config.config.syslog_config.priority = -1;  // 使用消息优先级
    
    if (linx_alert_add_output_config(&syslog_config) != 0) {
        printf("Failed to add syslog output config\n");
        result = -1;
    }
    
    return result;
}

/**
 * @brief 模拟规则匹配和告警发送
 */
static void simulate_alerts(linx_output_match_t *output_match)
{
    const char *test_rules[] = {
        "suspicious_process",
        "file_access_violation", 
        "network_anomaly",
        "privilege_escalation",
        "system_overload"
    };
    
    int priorities[] = {0, 1, 2, 3, 2};  // 对应不同的严重级别
    
    printf("\n=== Simulating alerts ===\n");
    
    for (int i = 0; i < 5; i++) {
        printf("Sending alert %d: %s (priority %d)\n", 
               i + 1, test_rules[i], priorities[i]);
               
        // 异步发送告警
        int result = linx_alert_format_and_send(output_match, test_rules[i], priorities[i]);
        if (result != 0) {
            printf("Failed to send alert %d\n", i + 1);
        }
        
        // 模拟一些间隔
        usleep(100000);  // 100ms
    }
    
    // 等待一段时间让线程池处理完所有任务
    printf("Waiting for alerts to be processed...\n");
    sleep(2);
}

/**
 * @brief 显示统计信息
 */
static void show_statistics(void)
{
    long total_sent, total_failed;
    linx_alert_get_stats(&total_sent, &total_failed);
    
    printf("\n=== Alert Statistics ===\n");
    printf("Total alerts sent: %ld\n", total_sent);
    printf("Total alerts failed: %ld\n", total_failed);
    printf("Success rate: %.2f%%\n", 
           total_sent + total_failed > 0 ? 
           (100.0 * total_sent) / (total_sent + total_failed) : 0.0);
}

/**
 * @brief 测试同步发送
 */
static void test_sync_sending(linx_output_match_t *output_match)
{
    printf("\n=== Testing synchronous sending ===\n");
    
    int result = linx_alert_send_sync(output_match, "sync_test_rule", 1);
    if (result == 0) {
        printf("Synchronous alert sent successfully\n");
    } else {
        printf("Failed to send synchronous alert\n");
    }
}

int main(int argc, char *argv[])
{
    printf("LINX Alert System Example\n");
    printf("========================\n");
    
    // 初始化告警系统
    printf("Initializing alert system...\n");
    if (linx_alert_init(4) != 0) {  // 使用4个工作线程
        printf("Failed to initialize alert system\n");
        return 1;
    }
    
    // 设置测试字段
    setup_test_fields();
    
    // 配置输出方式
    printf("Configuring outputs...\n");
    if (configure_outputs() != 0) {
        printf("Failed to configure some outputs\n");
    }
    
    // 创建测试输出匹配对象
    printf("Creating output format...\n");
    linx_output_match_t *output_match = create_test_output_match();
    if (!output_match) {
        printf("Failed to create output match\n");
        linx_alert_cleanup();
        return 1;
    }
    
    // 模拟告警发送
    simulate_alerts(output_match);
    
    // 测试同步发送
    test_sync_sending(output_match);
    
    // 显示统计信息
    show_statistics();
    
    // 清理资源
    printf("\nCleaning up...\n");
    
    // 清理输出匹配对象（您需要实现这个函数）
    // linx_output_match_destroy(output_match);
    
    // 清理告警系统
    linx_alert_cleanup();
    
    printf("Example completed successfully\n");
    return 0;
}