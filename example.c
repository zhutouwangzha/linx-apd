#define _GNU_SOURCE
#include "field_mapper.h"
#include <stddef.h>
#include <time.h>

// 定义类似falco的事件结构
typedef struct {
    int32_t pid;
    int32_t ppid;
    char name[256];
    char cmdline[1024];
    uint64_t timestamp;
    bool is_container;
} proc_info_t;

typedef struct {
    int32_t type;
    int32_t category;
    char direction[16];
    uint64_t timestamp;
    double duration;
    proc_info_t proc;
} event_info_t;

// 辅助函数：打印查询结果
void print_query_result(const char *field_name, field_query_result_t result) {
    if (!result.found) {
        printf("Field '%s' not found\n", field_name);
        return;
    }
    
    printf("Field '%s' found: ", field_name);
    
    switch (result.type) {
        case FIELD_TYPE_INT32:
            printf("(int32) %d\n", *(int32_t*)result.value_ptr);
            break;
        case FIELD_TYPE_INT64:
            printf("(int64) %ld\n", *(int64_t*)result.value_ptr);
            break;
        case FIELD_TYPE_UINT32:
            printf("(uint32) %u\n", *(uint32_t*)result.value_ptr);
            break;
        case FIELD_TYPE_UINT64:
            printf("(uint64) %lu\n", *(uint64_t*)result.value_ptr);
            break;
        case FIELD_TYPE_STRING:
            printf("(string) %s\n", (char*)result.value_ptr);
            break;
        case FIELD_TYPE_BOOL:
            printf("(bool) %s\n", *(bool*)result.value_ptr ? "true" : "false");
            break;
        case FIELD_TYPE_DOUBLE:
            printf("(double) %f\n", *(double*)result.value_ptr);
            break;
        case FIELD_TYPE_FLOAT:
            printf("(float) %f\n", *(float*)result.value_ptr);
            break;
        default:
            printf("(unknown type)\n");
            break;
    }
}

int main() {
    // 初始化映射器
    field_mapper_t *mapper = field_mapper_init();
    if (!mapper) {
        printf("Failed to initialize field mapper\n");
        return 1;
    }
    
    printf("=== Field Mapper Example ===\n\n");
    
    // 创建事件表并添加字段映射
    printf("Creating event table and adding field mappings...\n");
    field_mapper_create_table(mapper, "event");
    
    // 添加事件字段
    ADD_FIELD_MAPPING(mapper, "event", event_info_t, type, FIELD_TYPE_INT32);
    ADD_FIELD_MAPPING(mapper, "event", event_info_t, category, FIELD_TYPE_INT32);
    ADD_FIELD_MAPPING(mapper, "event", event_info_t, direction, FIELD_TYPE_STRING);
    ADD_FIELD_MAPPING(mapper, "event", event_info_t, timestamp, FIELD_TYPE_UINT64);
    ADD_FIELD_MAPPING(mapper, "event", event_info_t, duration, FIELD_TYPE_DOUBLE);
    
    // 添加进程字段（使用嵌套结构）
    field_mapper_add_field(mapper, "event", "proc.pid", 
                          offsetof(event_info_t, proc) + offsetof(proc_info_t, pid),
                          FIELD_TYPE_INT32, sizeof(int32_t));
    field_mapper_add_field(mapper, "event", "proc.ppid",
                          offsetof(event_info_t, proc) + offsetof(proc_info_t, ppid),
                          FIELD_TYPE_INT32, sizeof(int32_t));
    field_mapper_add_field(mapper, "event", "proc.name",
                          offsetof(event_info_t, proc) + offsetof(proc_info_t, name),
                          FIELD_TYPE_STRING, sizeof(((proc_info_t*)0)->name));
    field_mapper_add_field(mapper, "event", "proc.cmdline",
                          offsetof(event_info_t, proc) + offsetof(proc_info_t, cmdline),
                          FIELD_TYPE_STRING, sizeof(((proc_info_t*)0)->cmdline));
    field_mapper_add_field(mapper, "event", "proc.timestamp",
                          offsetof(event_info_t, proc) + offsetof(proc_info_t, timestamp),
                          FIELD_TYPE_UINT64, sizeof(uint64_t));
    field_mapper_add_field(mapper, "event", "proc.is_container",
                          offsetof(event_info_t, proc) + offsetof(proc_info_t, is_container),
                          FIELD_TYPE_BOOL, sizeof(bool));
    
    // 创建另一个表用于演示多表功能
    printf("Creating process table...\n");
    field_mapper_create_table(mapper, "process");
    ADD_FIELD_MAPPING(mapper, "process", proc_info_t, pid, FIELD_TYPE_INT32);
    ADD_FIELD_MAPPING(mapper, "process", proc_info_t, ppid, FIELD_TYPE_INT32);
    ADD_FIELD_MAPPING(mapper, "process", proc_info_t, name, FIELD_TYPE_STRING);
    ADD_FIELD_MAPPING(mapper, "process", proc_info_t, cmdline, FIELD_TYPE_STRING);
    ADD_FIELD_MAPPING(mapper, "process", proc_info_t, timestamp, FIELD_TYPE_UINT64);
    ADD_FIELD_MAPPING(mapper, "process", proc_info_t, is_container, FIELD_TYPE_BOOL);
    
    printf("\n");
    
    // 打印所有表信息
    field_mapper_print_all_tables(mapper);
    
    // 创建测试数据
    event_info_t test_event = {
        .type = 1,
        .category = 2,
        .direction = "ingress",
        .timestamp = 1234567890UL,
        .duration = 123.456,
        .proc = {
            .pid = 1234,
            .ppid = 1,
            .name = "test_process",
            .cmdline = "/usr/bin/test_process --arg1 --arg2",
            .timestamp = 1234567890UL,
            .is_container = true
        }
    };
    
    printf("=== Testing field queries ===\n");
    
    // 测试事件表查询
    printf("\nQuerying event table:\n");
    print_query_result("type", field_mapper_query_value(mapper, "event", "type", &test_event));
    print_query_result("category", field_mapper_query_value(mapper, "event", "category", &test_event));
    print_query_result("direction", field_mapper_query_value(mapper, "event", "direction", &test_event));
    print_query_result("timestamp", field_mapper_query_value(mapper, "event", "timestamp", &test_event));
    print_query_result("duration", field_mapper_query_value(mapper, "event", "duration", &test_event));
    
    // 测试嵌套字段查询
    printf("\nQuerying nested process fields:\n");
    print_query_result("proc.pid", field_mapper_query_value(mapper, "event", "proc.pid", &test_event));
    print_query_result("proc.ppid", field_mapper_query_value(mapper, "event", "proc.ppid", &test_event));
    print_query_result("proc.name", field_mapper_query_value(mapper, "event", "proc.name", &test_event));
    print_query_result("proc.cmdline", field_mapper_query_value(mapper, "event", "proc.cmdline", &test_event));
    print_query_result("proc.timestamp", field_mapper_query_value(mapper, "event", "proc.timestamp", &test_event));
    print_query_result("proc.is_container", field_mapper_query_value(mapper, "event", "proc.is_container", &test_event));
    
    // 测试进程表查询
    printf("\nQuerying process table:\n");
    print_query_result("pid", field_mapper_query_value(mapper, "process", "pid", &test_event.proc));
    print_query_result("name", field_mapper_query_value(mapper, "process", "name", &test_event.proc));
    print_query_result("cmdline", field_mapper_query_value(mapper, "process", "cmdline", &test_event.proc));
    print_query_result("is_container", field_mapper_query_value(mapper, "process", "is_container", &test_event.proc));
    
    // 测试不存在的字段
    printf("\nTesting non-existent fields:\n");
    print_query_result("nonexistent", field_mapper_query_value(mapper, "event", "nonexistent", &test_event));
    print_query_result("proc.nonexistent", field_mapper_query_value(mapper, "event", "proc.nonexistent", &test_event));
    
    // 性能测试
    printf("\n=== Performance Test ===\n");
    clock_t start = clock();
    const int iterations = 1000000;
    
    for (int i = 0; i < iterations; i++) {
        field_query_result_t result = field_mapper_query_value(mapper, "event", "proc.name", &test_event);
        // 避免编译器优化掉循环
        if (!result.found) break;
    }
    
    clock_t end = clock();
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Performed %d field lookups in %f seconds\n", iterations, cpu_time_used);
    printf("Average time per lookup: %f microseconds\n", (cpu_time_used * 1000000) / iterations);
    
    // 清理资源
    field_mapper_destroy(mapper);
    
    printf("\nExample completed successfully!\n");
    return 0;
}