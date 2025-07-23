#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "linx_hash_map.h"

/* 定义不同的结构体类型 */
typedef struct {
    int id;
    char name[32];
    float score;
} student_t;

typedef struct {
    int emp_id;
    char department[16];
    double salary;
} employee_t;

typedef struct {
    int product_id;
    char category[24];
    int stock;
    double price;
} product_t;

void setup_student_table(void)
{
    /* 创建学生表 */
    linx_hash_map_create_table("student", NULL);
    
    /* 添加字段信息 */
    linx_hash_map_add_field("student", "id", offsetof(student_t, id), 
                           sizeof(int), FIELD_TYPE_INT32);
    linx_hash_map_add_field("student", "name", offsetof(student_t, name), 
                           sizeof(char) * 32, FIELD_TYPE_CHARBUF);
    linx_hash_map_add_field("student", "score", offsetof(student_t, score), 
                           sizeof(float), FIELD_TYPE_FLOAT);
}

void setup_employee_table(void)
{
    /* 创建员工表 */
    linx_hash_map_create_table("employee", NULL);
    
    /* 添加字段信息 */
    linx_hash_map_add_field("employee", "emp_id", offsetof(employee_t, emp_id), 
                           sizeof(int), FIELD_TYPE_INT32);
    linx_hash_map_add_field("employee", "department", offsetof(employee_t, department), 
                           sizeof(char) * 16, FIELD_TYPE_CHARBUF);
    linx_hash_map_add_field("employee", "salary", offsetof(employee_t, salary), 
                           sizeof(double), FIELD_TYPE_DOUBLE);
}

void setup_product_table(void)
{
    /* 创建产品表 */
    linx_hash_map_create_table("product", NULL);
    
    /* 添加字段信息 */
    linx_hash_map_add_field("product", "product_id", offsetof(product_t, product_id), 
                           sizeof(int), FIELD_TYPE_INT32);
    linx_hash_map_add_field("product", "category", offsetof(product_t, category), 
                           sizeof(char) * 24, FIELD_TYPE_CHARBUF);
    linx_hash_map_add_field("product", "stock", offsetof(product_t, stock), 
                           sizeof(int), FIELD_TYPE_INT32);
    linx_hash_map_add_field("product", "price", offsetof(product_t, price), 
                           sizeof(double), FIELD_TYPE_DOUBLE);
}

void demonstrate_multi_struct_usage(void)
{
    /* 创建结构体实例 */
    student_t student = {1001, "张三", 85.5f};
    employee_t employee = {2001, "IT", 8500.0};
    product_t product = {3001, "电子产品", 100, 299.99};
    
    printf("=== 多结构体基地址管理演示 ===\n\n");
    
    /* 1. 更新各个表的基地址 */
    printf("1. 更新基地址:\n");
    if (linx_hash_map_update_base_addr("student", &student) == 0) {
        printf("   student表基地址更新成功: %p\n", &student);
    }
    if (linx_hash_map_update_base_addr("employee", &employee) == 0) {
        printf("   employee表基地址更新成功: %p\n", &employee);
    }
    if (linx_hash_map_update_base_addr("product", &product) == 0) {
        printf("   product表基地址更新成功: %p\n", &product);
    }
    printf("\n");
    
    /* 2. 列出所有表 */
    char **table_names;
    size_t table_count;
    
    printf("2. 系统中的所有表:\n");
    if (linx_hash_map_list_tables(&table_names, &table_count) == 0) {
        for (size_t i = 0; i < table_count; i++) {
            void *base_addr = linx_hash_map_get_base_addr(table_names[i]);
            printf("   表名: %-10s, 基地址: %p\n", table_names[i], base_addr);
        }
        linx_hash_map_free_table_list(table_names, table_count);
    }
    printf("\n");
    
    /* 3. 通过表名访问字段值 */
    printf("3. 通过表名访问字段值:\n");
    
    /* 访问学生数据 */
    field_result_t field = linx_hash_map_get_field("student", "id");
    if (field.found) {
        int *id_ptr = (int *)linx_hash_map_get_table_value_ptr("student", &field);
        printf("   student.id = %d\n", *id_ptr);
    }
    
    field = linx_hash_map_get_field("student", "name");
    if (field.found) {
        char *name_ptr = (char *)linx_hash_map_get_table_value_ptr("student", &field);
        printf("   student.name = %s\n", name_ptr);
    }
    
    /* 访问员工数据 */
    field = linx_hash_map_get_field("employee", "emp_id");
    if (field.found) {
        int *emp_id_ptr = (int *)linx_hash_map_get_table_value_ptr("employee", &field);
        printf("   employee.emp_id = %d\n", *emp_id_ptr);
    }
    
    field = linx_hash_map_get_field("employee", "salary");
    if (field.found) {
        double *salary_ptr = (double *)linx_hash_map_get_table_value_ptr("employee", &field);
        printf("   employee.salary = %.2f\n", *salary_ptr);
    }
    
    /* 访问产品数据 */
    field = linx_hash_map_get_field("product", "price");
    if (field.found) {
        double *price_ptr = (double *)linx_hash_map_get_table_value_ptr("product", &field);
        printf("   product.price = %.2f\n", *price_ptr);
    }
    printf("\n");
    
    /* 4. 模拟基地址变更（比如重新分配内存） */
    printf("4. 模拟基地址变更:\n");
    student_t *new_student = malloc(sizeof(student_t));
    *new_student = student;  /* 复制数据 */
    
    printf("   原基地址: %p -> 新基地址: %p\n", &student, new_student);
    
    /* 更新基地址 */
    linx_hash_map_update_base_addr("student", new_student);
    
    /* 验证新基地址下的数据访问 */
    field = linx_hash_map_get_field("student", "name");
    if (field.found) {
        char *name_ptr = (char *)linx_hash_map_get_table_value_ptr("student", &field);
        printf("   新地址下的 student.name = %s\n", name_ptr);
    }
    
    free(new_student);
    printf("\n");
    
    /* 5. 使用路径方式访问（需要重置基地址） */
    linx_hash_map_update_base_addr("student", &student);
    printf("5. 使用路径方式访问:\n");
    char path[] = "student.score";
    field = linx_hash_map_get_field_by_path(path);
    if (field.found) {
        float *score_ptr = (float *)linx_hash_map_get_table_value_ptr(field.table_name, &field);
        printf("   %s = %.1f\n", path, *score_ptr);
    }
}

int main(void)
{
    /* 初始化哈希映射 */
    if (linx_hash_map_init() != 0) {
        fprintf(stderr, "Failed to initialize linx_hash_map\n");
        return 1;
    }
    
    /* 设置各个表的结构信息 */
    setup_student_table();
    setup_employee_table();
    setup_product_table();
    
    /* 演示多结构体使用 */
    demonstrate_multi_struct_usage();
    
    /* 清理 */
    linx_hash_map_deinit();
    
    return 0;
}