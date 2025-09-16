#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "linx_rule_analyzer.h"
#include "linx_event_type.h"

/* 事件类型名称到枚举值的映射表 */
static const struct {
    const char *name;
    uint32_t type_enter;
    uint32_t type_exit;
} event_type_map[] = {
    {"read", LINX_EVENT_TYPE_READ_E, LINX_EVENT_TYPE_READ_X},
    {"write", LINX_EVENT_TYPE_WRITE_E, LINX_EVENT_TYPE_WRITE_X},
    {"open", LINX_EVENT_TYPE_OPEN_E, LINX_EVENT_TYPE_OPEN_X},
    {"openat", LINX_EVENT_TYPE_OPENAT_E, LINX_EVENT_TYPE_OPENAT_X},
    {"close", LINX_EVENT_TYPE_CLOSE_E, LINX_EVENT_TYPE_CLOSE_X},
    {"stat", LINX_EVENT_TYPE_STAT_E, LINX_EVENT_TYPE_STAT_X},
    {"fstat", LINX_EVENT_TYPE_FSTAT_E, LINX_EVENT_TYPE_FSTAT_X},
    {"lstat", LINX_EVENT_TYPE_LSTAT_E, LINX_EVENT_TYPE_LSTAT_X},
    {"execve", LINX_EVENT_TYPE_EXECVE_E, LINX_EVENT_TYPE_EXECVE_X},
    {"fork", LINX_EVENT_TYPE_FORK_E, LINX_EVENT_TYPE_FORK_X},
    {"clone", LINX_EVENT_TYPE_CLONE_E, LINX_EVENT_TYPE_CLONE_X},
    {"socket", LINX_EVENT_TYPE_SOCKET_E, LINX_EVENT_TYPE_SOCKET_X},
    {"connect", LINX_EVENT_TYPE_CONNECT_E, LINX_EVENT_TYPE_CONNECT_X},
    {"sendto", LINX_EVENT_TYPE_SENDTO_E, LINX_EVENT_TYPE_SENDTO_X},
    {"recvfrom", LINX_EVENT_TYPE_RECVFROM_E, LINX_EVENT_TYPE_RECVFROM_X},
    {"chmod", LINX_EVENT_TYPE_CHMOD_E, LINX_EVENT_TYPE_CHMOD_X},
    {"chown", LINX_EVENT_TYPE_CHOWN_E, LINX_EVENT_TYPE_CHOWN_X},
    {"mkdir", LINX_EVENT_TYPE_MKDIR_E, LINX_EVENT_TYPE_MKDIR_X},
    {"rmdir", LINX_EVENT_TYPE_RMDIR_E, LINX_EVENT_TYPE_RMDIR_X},
    {"unlink", LINX_EVENT_TYPE_UNLINK_E, LINX_EVENT_TYPE_UNLINK_X},
    {"rename", LINX_EVENT_TYPE_RENAME_E, LINX_EVENT_TYPE_RENAME_X},
    {"kill", LINX_EVENT_TYPE_KILL_E, LINX_EVENT_TYPE_KILL_X},
    {"exit", LINX_EVENT_TYPE_EXIT_E, LINX_EVENT_TYPE_EXIT_X},
    {"mmap", LINX_EVENT_TYPE_MMAP_E, LINX_EVENT_TYPE_MMAP_X},
    {"munmap", LINX_EVENT_TYPE_MUNMAP_E, LINX_EVENT_TYPE_MUNMAP_X},
    {"pipe", LINX_EVENT_TYPE_PIPE_E, LINX_EVENT_TYPE_PIPE_X},
    {"dup", LINX_EVENT_TYPE_DUP_E, LINX_EVENT_TYPE_DUP_X},
    {"dup2", LINX_EVENT_TYPE_DUP2_E, LINX_EVENT_TYPE_DUP2_X},
    {"setuid", LINX_EVENT_TYPE_SETUID_E, LINX_EVENT_TYPE_SETUID_X},
    {"setgid", LINX_EVENT_TYPE_SETGID_E, LINX_EVENT_TYPE_SETGID_X},
    {"ptrace", LINX_EVENT_TYPE_PTRACE_E, LINX_EVENT_TYPE_PTRACE_X},
};

/* 通用规则的关键词 */
static const char *generic_keywords[] = {
    "proc.name",
    "proc.cmdline", 
    "proc.pid",
    "proc.ppid",
    "user.name",
    "user.uid",
    "fd.name",
    "fd.path",
    NULL
};

/* 方向关键词 */
static const char *direction_keywords[] = {
    "evt.dir = <",    /* ENTER */
    "evt.dir = >",    /* EXIT */
    "evt.dir=\"<\"",  /* ENTER */
    "evt.dir=\">\"",  /* EXIT */
    NULL
};

int linx_rule_analyzer_init(void)
{
    /* 目前不需要特殊初始化 */
    return 0;
}

void linx_rule_analyzer_deinit(void)
{
    /* 目前不需要特殊清理 */
}

static char *linx_rule_normalize_condition(const char *condition)
{
    if (!condition) {
        return NULL;
    }
    
    size_t len = strlen(condition);
    char *normalized = malloc(len + 1);
    if (!normalized) {
        return NULL;
    }
    
    /* 转换为小写并去除多余空格 */
    size_t j = 0;
    bool in_space = true;
    
    for (size_t i = 0; i < len; i++) {
        char c = tolower(condition[i]);
        
        if (isspace(c)) {
            if (!in_space) {
                normalized[j++] = ' ';
                in_space = true;
            }
        } else {
            normalized[j++] = c;
            in_space = false;
        }
    }
    
    /* 去除尾部空格 */
    while (j > 0 && normalized[j-1] == ' ') {
        j--;
    }
    
    normalized[j] = '\0';
    return normalized;
}

int linx_rule_extract_event_types(const char *condition, uint32_t *event_types, size_t max_types)
{
    if (!condition || !event_types || max_types == 0) {
        return -1;
    }
    
    char *normalized = linx_rule_normalize_condition(condition);
    if (!normalized) {
        return -1;
    }
    
    size_t count = 0;
    size_t map_size = sizeof(event_type_map) / sizeof(event_type_map[0]);
    
    /* 查找 evt.type = xxx 或 evt.type in (xxx) 模式 */
    for (size_t i = 0; i < map_size && count < max_types; i++) {
        char pattern1[64];
        char pattern2[64];
        char pattern3[64];
        
        snprintf(pattern1, sizeof(pattern1), "evt.type = %s", event_type_map[i].name);
        snprintf(pattern2, sizeof(pattern2), "evt.type=%s", event_type_map[i].name);
        snprintf(pattern3, sizeof(pattern3), "evt.type in (%s", event_type_map[i].name);
        
        if (strstr(normalized, pattern1) || strstr(normalized, pattern2) || strstr(normalized, pattern3)) {
            /* 默认添加ENTER类型，如果没有明确指定方向 */
            event_types[count++] = event_type_map[i].type_enter;
            
            /* 如果有足够空间，也添加EXIT类型 */
            if (count < max_types) {
                event_types[count++] = event_type_map[i].type_exit;
            }
        }
    }
    
    free(normalized);
    return (int)count;
}

linx_event_direction_t linx_rule_extract_direction(const char *condition)
{
    if (!condition) {
        return LINX_EVENT_DIRECTION_MAX;
    }
    
    char *normalized = linx_rule_normalize_condition(condition);
    if (!normalized) {
        return LINX_EVENT_DIRECTION_MAX;
    }
    
    linx_event_direction_t direction = LINX_EVENT_DIRECTION_MAX;
    
    if (strstr(normalized, "evt.dir = <") || strstr(normalized, "evt.dir=\"<\"") || 
        strstr(normalized, "evt.dir=<")) {
        direction = LINX_EVENT_DIRECTION_ENTER;
    } else if (strstr(normalized, "evt.dir = >") || strstr(normalized, "evt.dir=\">\"") ||
               strstr(normalized, "evt.dir=>")) {
        direction = LINX_EVENT_DIRECTION_EXIT;
    }
    
    free(normalized);
    return direction;
}

bool linx_rule_is_generic(const char *condition)
{
    if (!condition) {
        return true;
    }
    
    char *normalized = linx_rule_normalize_condition(condition);
    if (!normalized) {
        return true;
    }
    
    /* 检查是否包含特定的事件类型 */
    size_t map_size = sizeof(event_type_map) / sizeof(event_type_map[0]);
    for (size_t i = 0; i < map_size; i++) {
        char pattern[64];
        snprintf(pattern, sizeof(pattern), "evt.type");
        if (strstr(normalized, pattern)) {
            /* 如果包含evt.type，则不是通用规则 */
            free(normalized);
            return false;
        }
    }
    
    /* 检查是否只包含通用字段 */
    for (int i = 0; generic_keywords[i]; i++) {
        if (strstr(normalized, generic_keywords[i])) {
            free(normalized);
            return true;
        }
    }
    
    free(normalized);
    /* 如果没有明确的分类信息，视为通用规则 */
    return true;
}

int linx_rule_analyze(const linx_rule_t *rule, linx_rule_analysis_t *analysis)
{
    if (!rule || !analysis) {
        return -1;
    }
    
    memset(analysis, 0, sizeof(linx_rule_analysis_t));
    
    if (!rule->condition) {
        analysis->is_generic = true;
        return 0;
    }
    
    /* 检查是否为通用规则 */
    if (linx_rule_is_generic(rule->condition)) {
        analysis->is_generic = true;
        return 0;
    }
    
    /* 提取事件类型 */
    uint32_t event_types[16];
    int type_count = linx_rule_extract_event_types(rule->condition, event_types, 16);
    
    if (type_count <= 0) {
        analysis->is_generic = true;
        return 0;
    }
    
    /* 提取事件方向 */
    linx_event_direction_t direction = linx_rule_extract_direction(rule->condition);
    
    /* 使用第一个事件类型进行分类 */
    analysis->classification.type = event_types[0];
    analysis->classification.source = linx_event_type_to_source(event_types[0]);
    
    if (direction != LINX_EVENT_DIRECTION_MAX) {
        analysis->classification.direction = direction;
    } else {
        analysis->classification.direction = linx_event_type_to_direction(event_types[0]);
    }
    
    analysis->has_classification = true;
    analysis->is_generic = false;
    
    return 0;
}