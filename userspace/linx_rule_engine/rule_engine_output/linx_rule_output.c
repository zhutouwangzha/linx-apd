#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "linx_rule_output.h"

// 定义最大输出缓冲区大小
#define MAX_OUTPUT_BUFFER_SIZE 4096
#define MAX_FIELD_VALUE_SIZE 256

/**
 * @brief 获取当前时间字符串
 */
static void get_current_time(char *time_str, size_t size) {
    struct timespec ts;
    struct tm *tm_info;
    
    clock_gettime(CLOCK_REALTIME, &ts);
    tm_info = localtime(&ts.tv_sec);
    
    snprintf(time_str, size, "%04d-%02d-%02d %02d:%02d:%02d.%03ld",
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
             ts.tv_nsec / 1000000);
}

/**
 * @brief 获取当前用户名
 */
static void get_current_user(char *user_str, size_t size) {
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        snprintf(user_str, size, "%s", pw->pw_name);
    } else {
        snprintf(user_str, size, "unknown");
    }
}

/**
 * @brief 获取当前进程名
 */
static void get_current_process_name(char *proc_name, size_t size) {
    FILE *fp = fopen("/proc/self/comm", "r");
    if (fp) {
        if (fgets(proc_name, size, fp)) {
            // 移除换行符
            char *newline = strchr(proc_name, '\n');
            if (newline) *newline = '\0';
        }
        fclose(fp);
    } else {
        snprintf(proc_name, size, "unknown");
    }
}

/**
 * @brief 获取当前进程ID
 */
static void get_current_process_id(char *pid_str, size_t size) {
    snprintf(pid_str, size, "%d", getpid());
}

/**
 * @brief 获取父进程ID
 */
static void get_parent_process_id(char *ppid_str, size_t size) {
    snprintf(ppid_str, size, "%d", getppid());
}

/**
 * @brief 获取进程命令行
 */
static void get_process_cmdline(char *cmdline, size_t size) {
    FILE *fp = fopen("/proc/self/cmdline", "r");
    if (fp) {
        size_t len = fread(cmdline, 1, size - 1, fp);
        cmdline[len] = '\0';
        
        // 将NULL字符替换为空格
        for (size_t i = 0; i < len; i++) {
            if (cmdline[i] == '\0') {
                cmdline[i] = ' ';
            }
        }
        fclose(fp);
    } else {
        snprintf(cmdline, size, "unknown");
    }
}

/**
 * @brief 替换字符串中的变量
 */
static char* replace_variable(const char *input, const char *variable, const char *value) {
    if (!input || !variable || !value) {
        return NULL;
    }
    
    size_t input_len = strlen(input);
    size_t var_len = strlen(variable);
    size_t val_len = strlen(value);
    
    // 计算需要替换的次数
    int count = 0;
    const char *pos = input;
    while ((pos = strstr(pos, variable)) != NULL) {
        count++;
        pos += var_len;
    }
    
    if (count == 0) {
        return strdup(input);
    }
    
    // 计算新字符串长度
    size_t new_len = input_len + count * (val_len - var_len) + 1;
    char *result = malloc(new_len);
    if (!result) {
        return NULL;
    }
    
    // 执行替换
    char *dest = result;
    const char *src = input;
    
    while ((pos = strstr(src, variable)) != NULL) {
        // 复制变量前的部分
        size_t prefix_len = pos - src;
        memcpy(dest, src, prefix_len);
        dest += prefix_len;
        
        // 复制替换值
        memcpy(dest, value, val_len);
        dest += val_len;
        
        // 移动源指针
        src = pos + var_len;
    }
    
    // 复制剩余部分
    strcpy(dest, src);
    
    return result;
}

int linx_rule_output_format_string(const char *output_template, char **formatted_output) {
    if (!output_template || !formatted_output) {
        return -1;
    }
    
    char time_str[64];
    char user_str[MAX_FIELD_VALUE_SIZE];
    char proc_name[MAX_FIELD_VALUE_SIZE];
    char pid_str[32];
    char ppid_str[32];
    char cmdline[MAX_FIELD_VALUE_SIZE];
    
    // 获取各种字段的值
    get_current_time(time_str, sizeof(time_str));
    get_current_user(user_str, sizeof(user_str));
    get_current_process_name(proc_name, sizeof(proc_name));
    get_current_process_id(pid_str, sizeof(pid_str));
    get_parent_process_id(ppid_str, sizeof(ppid_str));
    get_process_cmdline(cmdline, sizeof(cmdline));
    
    // 开始替换变量
    char *result = strdup(output_template);
    if (!result) {
        return -1;
    }
    
    // 定义变量替换映射
    struct {
        const char *variable;
        const char *value;
    } replacements[] = {
        {"%evt.time", time_str},
        {"%user.name", user_str},
        {"%proc.name", proc_name},
        {"%proc.pid", pid_str},
        {"%proc.ppid", ppid_str},
        {"%proc.cmdline", cmdline},
        {"%fd.name", "/etc/passwd"},  // 临时使用固定值，实际应该从事件中获取
        {"%evt.arg.data", "sample_data"},  // 临时使用固定值，实际应该从事件中获取
        {NULL, NULL}
    };
    
    // 逐个替换变量
    for (int i = 0; replacements[i].variable != NULL; i++) {
        char *new_result = replace_variable(result, replacements[i].variable, replacements[i].value);
        if (new_result) {
            free(result);
            result = new_result;
        }
    }
    
    *formatted_output = result;
    return 0;
}

int linx_rule_output_format_and_print(const linx_rule_t *rule) {
    if (!rule || !rule->output) {
        return -1;
    }
    
    char *formatted_output = NULL;
    int ret = linx_rule_output_format_string(rule->output, &formatted_output);
    
    if (ret == 0 && formatted_output) {
        // 输出格式化后的结果
        printf("%s\n", formatted_output);
        fflush(stdout);
        
        free(formatted_output);
        return 0;
    }
    
    return -1;
}