#define _GNU_SOURCE
#include "field_mapper.h"
#include <stddef.h>
#include <time.h>

// 定义进程相关结构体
typedef struct {
    int32_t pid;
    int32_t ppid;
    int32_t uid;
    int32_t gid;
    char name[256];
    char cmdline[1024];
    char exe[512];
    bool is_container;
    uint64_t start_time;
} proc_info_t;

// 定义事件相关结构体
typedef struct {
    int32_t type;
    int32_t category;
    uint64_t timestamp;
    char direction[16];
    int32_t retval;
    uint32_t tid;
    double duration;
} evt_info_t;

// 定义文件相关结构体
typedef struct {
    char path[512];
    char name[256];
    uint32_t fd;
    uint32_t flags;
    uint64_t size;
    uint32_t mode;
} file_info_t;

// 辅助函数：打印查询结果
void print_query_result(const char *table_field, field_query_result_t result) {
    if (!result.found) {
        printf("❌ Field '%s' not found\n", table_field);
        return;
    }
    
    printf("✅ %s.%s: ", result.table_id, result.field_name);
    
    switch (result.type) {
        case FIELD_TYPE_INT32:
            printf("(int32) %d\n", *(int32_t*)result.value_ptr);
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
        default:
            printf("(unknown type)\n");
            break;
    }
}

int main() {
    printf("=== New Hash Map Manager Example ===\n\n");
    
    // 初始化hash_map管理器，启用自动扩容，阈值75%
    hash_map_manager_t *manager = hash_map_manager_init(true, 75);
    if (!manager) {
        printf("Failed to initialize hash map manager\n");
        return 1;
    }
    
    printf("🚀 Hash Map Manager initialized with auto-expand enabled\n\n");
    
    // ==================== 创建proc表 ====================
    printf("📋 Creating 'proc' table for process-related fields...\n");
    int result = hash_map_manager_create_table(manager, "proc", 
                                             "Process information fields", 64);
    if (result != HASH_MAP_SUCCESS) {
        printf("Failed to create proc table: %d\n", result);
        return 1;
    }
    
    // 使用批量添加方式添加proc字段
    field_mapping_t proc_mappings[] = {
        {"pid", FIELD_OFFSET(proc_info_t, pid), FIELD_TYPE_INT32, sizeof(int32_t)},
        {"ppid", FIELD_OFFSET(proc_info_t, ppid), FIELD_TYPE_INT32, sizeof(int32_t)},
        {"uid", FIELD_OFFSET(proc_info_t, uid), FIELD_TYPE_INT32, sizeof(int32_t)},
        {"gid", FIELD_OFFSET(proc_info_t, gid), FIELD_TYPE_INT32, sizeof(int32_t)},
        {"name", FIELD_OFFSET(proc_info_t, name), FIELD_TYPE_STRING, sizeof(((proc_info_t*)0)->name)},
        {"cmdline", FIELD_OFFSET(proc_info_t, cmdline), FIELD_TYPE_STRING, sizeof(((proc_info_t*)0)->cmdline)},
        {"exe", FIELD_OFFSET(proc_info_t, exe), FIELD_TYPE_STRING, sizeof(((proc_info_t*)0)->exe)},
        {"is_container", FIELD_OFFSET(proc_info_t, is_container), FIELD_TYPE_BOOL, sizeof(bool)},
        {"start_time", FIELD_OFFSET(proc_info_t, start_time), FIELD_TYPE_UINT64, sizeof(uint64_t)},
    };
    size_t mapping_count = sizeof(proc_mappings) / sizeof(proc_mappings[0]);
    
    result = hash_map_manager_add_fields_batch(manager, "proc", proc_mappings, mapping_count);
    if (result != HASH_MAP_SUCCESS) {
        printf("Failed to add proc fields: %d\n", result);
        return 1;
    }
    printf("✅ Added %zu fields to 'proc' table\n", mapping_count);
    
    // ==================== 创建evt表 ====================
    printf("📋 Creating 'evt' table for event-related fields...\n");
    result = hash_map_manager_create_table(manager, "evt", 
                                         "Event information fields", 32);
    if (result != HASH_MAP_SUCCESS) {
        printf("Failed to create evt table: %d\n", result);
        return 1;
    }
    
    // 使用单个添加方式添加evt字段
    ADD_FIELD_TO_TABLE(manager, "evt", evt_info_t, type, FIELD_TYPE_INT32);
    ADD_FIELD_TO_TABLE(manager, "evt", evt_info_t, category, FIELD_TYPE_INT32);
    ADD_FIELD_TO_TABLE(manager, "evt", evt_info_t, timestamp, FIELD_TYPE_UINT64);
    ADD_FIELD_TO_TABLE(manager, "evt", evt_info_t, direction, FIELD_TYPE_STRING);
    ADD_FIELD_TO_TABLE(manager, "evt", evt_info_t, retval, FIELD_TYPE_INT32);
    ADD_FIELD_TO_TABLE(manager, "evt", evt_info_t, tid, FIELD_TYPE_UINT32);
    ADD_FIELD_TO_TABLE(manager, "evt", evt_info_t, duration, FIELD_TYPE_DOUBLE);
    printf("✅ Added 7 fields to 'evt' table\n");
    
    // ==================== 创建file表 ====================
    printf("📋 Creating 'file' table for file-related fields...\n");
    result = hash_map_manager_create_table(manager, "file", 
                                         "File information fields", 16);
    if (result != HASH_MAP_SUCCESS) {
        printf("Failed to create file table: %d\n", result);
        return 1;
    }
    
    ADD_FIELD_TO_TABLE(manager, "file", file_info_t, path, FIELD_TYPE_STRING);
    ADD_FIELD_TO_TABLE(manager, "file", file_info_t, name, FIELD_TYPE_STRING);
    ADD_FIELD_TO_TABLE(manager, "file", file_info_t, fd, FIELD_TYPE_UINT32);
    ADD_FIELD_TO_TABLE(manager, "file", file_info_t, flags, FIELD_TYPE_UINT32);
    ADD_FIELD_TO_TABLE(manager, "file", file_info_t, size, FIELD_TYPE_UINT64);
    ADD_FIELD_TO_TABLE(manager, "file", file_info_t, mode, FIELD_TYPE_UINT32);
    printf("✅ Added 6 fields to 'file' table\n\n");
    
    // ==================== 打印管理器信息 ====================
    hash_map_manager_print_info(manager);
    printf("\n");
    
    // ==================== 创建测试数据 ====================
    proc_info_t test_proc = {
        .pid = 1234,
        .ppid = 1,
        .uid = 1000,
        .gid = 1000,
        .name = "test_process",
        .cmdline = "/usr/bin/test_process --config /etc/test.conf",
        .exe = "/usr/bin/test_process",
        .is_container = true,
        .start_time = 1234567890UL
    };
    
    evt_info_t test_evt = {
        .type = 1,
        .category = 2,
        .timestamp = 1234567890UL,
        .direction = "ingress",
        .retval = 0,
        .tid = 1234,
        .duration = 123.456
    };
    
    file_info_t test_file = {
        .path = "/etc/passwd",
        .name = "passwd",
        .fd = 3,
        .flags = 0,
        .size = 2048,
        .mode = 644
    };
    
    // ==================== 测试字段查询 ====================
    printf("🔍 Testing field queries:\n\n");
    
    // 查询proc表字段
    printf("📊 Proc table queries:\n");
    print_query_result("proc.pid", hash_map_manager_query_field(manager, "proc", "pid", &test_proc));
    print_query_result("proc.name", hash_map_manager_query_field(manager, "proc", "name", &test_proc));
    print_query_result("proc.cmdline", hash_map_manager_query_field(manager, "proc", "cmdline", &test_proc));
    print_query_result("proc.is_container", hash_map_manager_query_field(manager, "proc", "is_container", &test_proc));
    print_query_result("proc.start_time", hash_map_manager_query_field(manager, "proc", "start_time", &test_proc));
    printf("\n");
    
    // 查询evt表字段
    printf("📊 Event table queries:\n");
    print_query_result("evt.type", hash_map_manager_query_field(manager, "evt", "type", &test_evt));
    print_query_result("evt.timestamp", hash_map_manager_query_field(manager, "evt", "timestamp", &test_evt));
    print_query_result("evt.direction", hash_map_manager_query_field(manager, "evt", "direction", &test_evt));
    print_query_result("evt.duration", hash_map_manager_query_field(manager, "evt", "duration", &test_evt));
    printf("\n");
    
    // 查询file表字段
    printf("📊 File table queries:\n");
    print_query_result("file.path", hash_map_manager_query_field(manager, "file", "path", &test_file));
    print_query_result("file.name", hash_map_manager_query_field(manager, "file", "name", &test_file));
    print_query_result("file.size", hash_map_manager_query_field(manager, "file", "size", &test_file));
    print_query_result("file.mode", hash_map_manager_query_field(manager, "file", "mode", &test_file));
    printf("\n");
    
    // ==================== 测试扩容功能 ====================
    printf("🔧 Testing auto-expand functionality:\n");
    
    // 获取当前负载因子
    double load_factor = hash_map_manager_get_load_factor(manager, "file");
    printf("Current 'file' table load factor: %.1f%%\n", load_factor);
    
    // 添加更多字段触发扩容
    printf("Adding more fields to trigger auto-expand...\n");
    for (int i = 0; i < 20; i++) {
        char field_name[32];
        snprintf(field_name, sizeof(field_name), "extra_field_%d", i);
        hash_map_manager_add_field(manager, "file", field_name, 
                                  sizeof(file_info_t) + i * 4, FIELD_TYPE_INT32, 4);
    }
    
    // 检查扩容后的状态
    load_factor = hash_map_manager_get_load_factor(manager, "file");
    printf("After adding fields, 'file' table load factor: %.1f%%\n", load_factor);
    printf("\n");
    
    // ==================== 获取统计信息 ====================
    printf("📈 Manager Statistics:\n");
    manager_stats_t *stats = hash_map_manager_get_stats(manager);
    if (stats) {
        printf("Total tables: %zu\n", stats->table_count);
        printf("Total fields: %zu\n", stats->total_fields);
        printf("Total capacity: %zu\n", stats->total_capacity);
        printf("Average load factor: %.1f%%\n", stats->avg_load_factor);
        printf("\nTable details:\n");
        
        for (size_t i = 0; i < stats->table_count; i++) {
            printf("  %s: %zu fields, capacity %zu, load %.1f%%\n",
                   stats->table_stats[i].table_id,
                   stats->table_stats[i].field_count,
                   stats->table_stats[i].capacity,
                   stats->table_stats[i].load_factor);
        }
        
        hash_map_manager_free_stats(stats);
    }
    printf("\n");
    
    // ==================== 列出表和字段 ====================
    printf("📋 Listing all tables and fields:\n");
    size_t table_count;
    char **table_list = hash_map_manager_list_tables(manager, &table_count);
    if (table_list) {
        for (size_t i = 0; i < table_count; i++) {
            printf("Table '%s':\n", table_list[i]);
            
            size_t field_count;
            char **field_list = hash_map_manager_list_fields(manager, table_list[i], &field_count);
            if (field_list) {
                printf("  Fields (%zu): ", field_count);
                for (size_t j = 0; j < field_count; j++) {
                    printf("%s", field_list[j]);
                    if (j < field_count - 1) printf(", ");
                    free(field_list[j]);
                }
                printf("\n");
                free(field_list);
            }
            free(table_list[i]);
        }
        free(table_list);
    }
    printf("\n");
    
    // ==================== 性能测试 ====================
    printf("⚡ Performance Test:\n");
    clock_t start = clock();
    const int iterations = 1000000;
    
    for (int i = 0; i < iterations; i++) {
        field_query_result_t result = hash_map_manager_query_field(manager, "proc", "name", &test_proc);
        // 避免编译器优化掉循环
        if (!result.found) break;
    }
    
    clock_t end = clock();
    double cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Performed %d field lookups in %f seconds\n", iterations, cpu_time_used);
    printf("Average time per lookup: %.2f nanoseconds\n", (cpu_time_used * 1000000000) / iterations);
    printf("Queries per second: %.0f\n", iterations / cpu_time_used);
    printf("\n");
    
    // ==================== 测试表管理 ====================
    printf("🗑️  Testing table management:\n");
    
    // 删除file表
    result = hash_map_manager_remove_table(manager, "file");
    if (result == HASH_MAP_SUCCESS) {
        printf("✅ Successfully removed 'file' table\n");
    } else {
        printf("❌ Failed to remove 'file' table: %d\n", result);
    }
    
    // 验证表已被删除
    bool exists = hash_map_manager_table_exists(manager, "file");
    printf("'file' table exists: %s\n", exists ? "yes" : "no");
    
    // 最终状态
    printf("\nFinal manager state:\n");
    hash_map_manager_print_info(manager);
    
    // 清理资源
    hash_map_manager_destroy(manager);
    
    printf("\n✨ Example completed successfully!\n");
    return 0;
}