#define _GNU_SOURCE
#include "field_mapper.h"
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <time.h>

// 测试结构
typedef struct {
    int32_t id;
    char name[64];
    double value;
    bool active;
    struct {
        int32_t x;
        int32_t y;
    } point;
} test_struct_t;

// 测试计数器
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("✅ PASS: %s\n", message); \
            tests_passed++; \
        } else { \
            printf("❌ FAIL: %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

void test_basic_functionality() {
    printf("\n=== Testing Basic Functionality ===\n");
    
    // 初始化映射器
    field_mapper_t *mapper = field_mapper_init();
    TEST_ASSERT(mapper != NULL, "field_mapper_init() should return non-NULL");
    
    // 创建表
    int result = field_mapper_create_table(mapper, "test_table");
    TEST_ASSERT(result == 0, "field_mapper_create_table() should return 0");
    
    // 重复创建表应该成功
    result = field_mapper_create_table(mapper, "test_table");
    TEST_ASSERT(result == 0, "Creating existing table should return 0");
    
    // 添加字段映射
    result = field_mapper_add_field(mapper, "test_table", "id", 
                                   offsetof(test_struct_t, id),
                                   FIELD_TYPE_INT32, sizeof(int32_t));
    TEST_ASSERT(result == 0, "field_mapper_add_field() should return 0");
    
    // 查询字段信息
    field_info_t *field_info = field_mapper_get_field_info(mapper, "test_table", "id");
    TEST_ASSERT(field_info != NULL, "field_mapper_get_field_info() should return non-NULL");
    TEST_ASSERT(field_info->offset == offsetof(test_struct_t, id), "Field offset should be correct");
    TEST_ASSERT(field_info->type == FIELD_TYPE_INT32, "Field type should be correct");
    
    // 清理
    field_mapper_destroy(mapper);
    printf("Basic functionality tests completed\n");
}

void test_field_queries() {
    printf("\n=== Testing Field Queries ===\n");
    
    field_mapper_t *mapper = field_mapper_init();
    field_mapper_create_table(mapper, "test");
    
    // 添加各种类型的字段
    ADD_FIELD_MAPPING(mapper, "test", test_struct_t, id, FIELD_TYPE_INT32);
    ADD_FIELD_MAPPING(mapper, "test", test_struct_t, name, FIELD_TYPE_STRING);
    ADD_FIELD_MAPPING(mapper, "test", test_struct_t, value, FIELD_TYPE_DOUBLE);
    ADD_FIELD_MAPPING(mapper, "test", test_struct_t, active, FIELD_TYPE_BOOL);
    
    // 添加嵌套字段
    field_mapper_add_field(mapper, "test", "point.x",
                          offsetof(test_struct_t, point.x),
                          FIELD_TYPE_INT32, sizeof(int32_t));
    field_mapper_add_field(mapper, "test", "point.y",
                          offsetof(test_struct_t, point.y),
                          FIELD_TYPE_INT32, sizeof(int32_t));
    
    // 创建测试数据
    test_struct_t test_data = {
        .id = 42,
        .name = "test_object",
        .value = 3.14159,
        .active = true,
        .point = { .x = 100, .y = 200 }
    };
    
    // 测试查询各种类型
    field_query_result_t result;
    
    // 测试整数字段
    result = field_mapper_query_value(mapper, "test", "id", &test_data);
    TEST_ASSERT(result.found, "Should find 'id' field");
    TEST_ASSERT(result.type == FIELD_TYPE_INT32, "Type should be INT32");
    TEST_ASSERT(*(int32_t*)result.value_ptr == 42, "Value should be 42");
    
    // 测试字符串字段
    result = field_mapper_query_value(mapper, "test", "name", &test_data);
    TEST_ASSERT(result.found, "Should find 'name' field");
    TEST_ASSERT(result.type == FIELD_TYPE_STRING, "Type should be STRING");
    TEST_ASSERT(strcmp((char*)result.value_ptr, "test_object") == 0, "String value should match");
    
    // 测试浮点字段
    result = field_mapper_query_value(mapper, "test", "value", &test_data);
    TEST_ASSERT(result.found, "Should find 'value' field");
    TEST_ASSERT(result.type == FIELD_TYPE_DOUBLE, "Type should be DOUBLE");
    TEST_ASSERT(*(double*)result.value_ptr == 3.14159, "Double value should match");
    
    // 测试布尔字段
    result = field_mapper_query_value(mapper, "test", "active", &test_data);
    TEST_ASSERT(result.found, "Should find 'active' field");
    TEST_ASSERT(result.type == FIELD_TYPE_BOOL, "Type should be BOOL");
    TEST_ASSERT(*(bool*)result.value_ptr == true, "Bool value should be true");
    
    // 测试嵌套字段
    result = field_mapper_query_value(mapper, "test", "point.x", &test_data);
    TEST_ASSERT(result.found, "Should find 'point.x' field");
    TEST_ASSERT(*(int32_t*)result.value_ptr == 100, "point.x should be 100");
    
    result = field_mapper_query_value(mapper, "test", "point.y", &test_data);
    TEST_ASSERT(result.found, "Should find 'point.y' field");
    TEST_ASSERT(*(int32_t*)result.value_ptr == 200, "point.y should be 200");
    
    field_mapper_destroy(mapper);
    printf("Field query tests completed\n");
}

void test_error_conditions() {
    printf("\n=== Testing Error Conditions ===\n");
    
    field_mapper_t *mapper = field_mapper_init();
    field_mapper_create_table(mapper, "test");
    
    // 测试查询不存在的字段
    field_query_result_t result = field_mapper_query_value(mapper, "test", "nonexistent", NULL);
    TEST_ASSERT(!result.found, "Should not find nonexistent field");
    
    // 测试查询不存在的表
    result = field_mapper_query_value(mapper, "nonexistent_table", "field", NULL);
    TEST_ASSERT(!result.found, "Should not find field in nonexistent table");
    
    // 测试NULL参数
    field_info_t *field_info = field_mapper_get_field_info(NULL, "test", "field");
    TEST_ASSERT(field_info == NULL, "Should return NULL for NULL mapper");
    
    field_info = field_mapper_get_field_info(mapper, NULL, "field");
    TEST_ASSERT(field_info == NULL, "Should return NULL for NULL table name");
    
    field_info = field_mapper_get_field_info(mapper, "test", NULL);
    TEST_ASSERT(field_info == NULL, "Should return NULL for NULL field name");
    
    field_mapper_destroy(mapper);
    printf("Error condition tests completed\n");
}

void test_multiple_tables() {
    printf("\n=== Testing Multiple Tables ===\n");
    
    field_mapper_t *mapper = field_mapper_init();
    
    // 创建多个表
    field_mapper_create_table(mapper, "table1");
    field_mapper_create_table(mapper, "table2");
    field_mapper_create_table(mapper, "table3");
    
    // 在不同表中添加同名字段
    field_mapper_add_field(mapper, "table1", "common_field", 0, FIELD_TYPE_INT32, 4);
    field_mapper_add_field(mapper, "table2", "common_field", 4, FIELD_TYPE_STRING, 64);
    field_mapper_add_field(mapper, "table3", "common_field", 8, FIELD_TYPE_DOUBLE, 8);
    
    // 验证字段在不同表中的信息
    field_info_t *field1 = field_mapper_get_field_info(mapper, "table1", "common_field");
    field_info_t *field2 = field_mapper_get_field_info(mapper, "table2", "common_field");
    field_info_t *field3 = field_mapper_get_field_info(mapper, "table3", "common_field");
    
    TEST_ASSERT(field1 != NULL && field1->offset == 0 && field1->type == FIELD_TYPE_INT32,
               "table1 field should have correct properties");
    TEST_ASSERT(field2 != NULL && field2->offset == 4 && field2->type == FIELD_TYPE_STRING,
               "table2 field should have correct properties");
    TEST_ASSERT(field3 != NULL && field3->offset == 8 && field3->type == FIELD_TYPE_DOUBLE,
               "table3 field should have correct properties");
    
    field_mapper_destroy(mapper);
    printf("Multiple tables tests completed\n");
}

void test_performance() {
    printf("\n=== Testing Performance ===\n");
    
    field_mapper_t *mapper = field_mapper_init();
    field_mapper_create_table(mapper, "perf_test");
    
    // 添加大量字段
    const int field_count = 1000;
    char field_name[64];
    
    clock_t start = clock();
    for (int i = 0; i < field_count; i++) {
        snprintf(field_name, sizeof(field_name), "field_%d", i);
        field_mapper_add_field(mapper, "perf_test", field_name, i * 4, FIELD_TYPE_INT32, 4);
    }
    clock_t end = clock();
    
    double add_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Added %d fields in %f seconds (%.2f fields/sec)\n", 
           field_count, add_time, field_count / add_time);
    
    // 测试查询性能
    test_struct_t test_data = { .id = 42 };
    const int query_count = 100000;
    
    start = clock();
    for (int i = 0; i < query_count; i++) {
        snprintf(field_name, sizeof(field_name), "field_%d", i % field_count);
        field_query_result_t result = field_mapper_query_value(mapper, "perf_test", field_name, &test_data);
        // 避免编译器优化
        if (!result.found) break;
    }
    end = clock();
    
    double query_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Performed %d queries in %f seconds (%.2f queries/sec)\n",
           query_count, query_time, query_count / query_time);
    printf("Average query time: %.2f nanoseconds\n", (query_time * 1000000000) / query_count);
    
    TEST_ASSERT(query_time < 1.0, "Query performance should be reasonable");
    
    field_mapper_destroy(mapper);
    printf("Performance tests completed\n");
}

int main() {
    printf("=== Field Mapper Unit Tests ===\n");
    
    test_basic_functionality();
    test_field_queries();
    test_error_conditions();
    test_multiple_tables();
    test_performance();
    
    printf("\n=== Test Summary ===\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("Total tests: %d\n", tests_passed + tests_failed);
    
    if (tests_failed == 0) {
        printf("🎉 All tests passed!\n");
        return 0;
    } else {
        printf("💥 Some tests failed!\n");
        return 1;
    }
}