#ifndef __LINX_EVENT_DATA_H__
#define __LINX_EVENT_DATA_H__

#define _GNU_SOURCE
#include <stdint.h>
#include <sys/types.h>
#include <stddef.h>

// 时间结构体定义
struct linx_timespec {
    long tv_sec;
    long tv_nsec;
};

/**
 * @brief 事件数据结构体
 * 
 * 这个结构体包含了规则匹配时需要的所有字段信息
 * 字段的偏移量在编译时确定，运行时直接使用偏移量访问
 */
typedef struct {
    // 时间相关字段
    struct linx_timespec timestamp;     // %evt.time
    
    // 进程相关字段
    pid_t pid;                          // %proc.pid
    pid_t ppid;                         // %proc.ppid
    uid_t uid;                          // %user.uid
    char *process_name;                 // %proc.name
    char *cmdline;                      // %proc.cmdline
    char *username;                     // %user.name
    
    // 文件相关字段
    char *file_name;                    // %fd.name
    char *file_path;                    // %fd.path
    uint32_t file_type;                 // %fd.type
    
    // 事件相关字段
    uint32_t event_type;                // %evt.type
    char *event_direction;              // %evt.dir
    char *event_data;                   // %evt.arg.data
    int32_t event_result;               // %evt.res
    
    // 网络相关字段
    char *src_ip;                       // %net.src_ip
    char *dst_ip;                       // %net.dst_ip
    uint16_t src_port;                  // %net.src_port
    uint16_t dst_port;                  // %net.dst_port
    char *protocol;                     // %net.proto
    
    // 容器相关字段
    char *container_id;                 // %container.id
    char *container_name;               // %container.name
    
} linx_event_data_t;

/**
 * @brief 创建事件数据结构体
 * 
 * @return linx_event_data_t* 新创建的事件数据指针，失败返回NULL
 */
linx_event_data_t *linx_event_data_create(void);

/**
 * @brief 销毁事件数据结构体
 * 
 * @param event_data 要销毁的事件数据指针
 */
void linx_event_data_destroy(linx_event_data_t *event_data);

/**
 * @brief 设置事件数据的字符串字段
 * 
 * @param event_data 事件数据指针
 * @param field_name 字段名称
 * @param value 字符串值
 * @return int 0表示成功，-1表示失败
 */
int linx_event_data_set_string(linx_event_data_t *event_data, const char *field_name, const char *value);

/**
 * @brief 设置事件数据的整数字段
 * 
 * @param event_data 事件数据指针
 * @param field_name 字段名称
 * @param value 整数值
 * @return int 0表示成功，-1表示失败
 */
int linx_event_data_set_int(linx_event_data_t *event_data, const char *field_name, int64_t value);

/**
 * @brief 设置事件数据的时间戳字段
 * 
 * @param event_data 事件数据指针
 * @return int 0表示成功，-1表示失败
 */
int linx_event_data_set_current_time(linx_event_data_t *event_data);

// 字段偏移量宏定义，用于快速访问
#define LINX_EVENT_FIELD_OFFSET(field) offsetof(linx_event_data_t, field)
#define LINX_EVENT_FIELD_SIZE(field) sizeof(((linx_event_data_t*)0)->field)

#endif /* __LINX_EVENT_DATA_H__ */