#ifndef __LINX_PROCESS_CACHE_NODE_H__
#define __LINX_PROCESS_CACHE_NODE_H__ 

#include <sys/types.h>

/**
 * 计划有这些字段
 * pid  产生事件的进程ID
 * exe  第一个命令行参数（通常是可执行文件名称或自定义名称）
 * name 生成事件的可执行文件的名称（不包括路径）
 * args 当启动生成事件的进程时，在命令行上传递的参数
 * env  生成事件的进程的环境变量
 * cwd  事件的当前工作目录
 * ppid 事件进程的父进程PID
 * pname 父进程的名称（不包括路径）
 * pcmdline 
 * tty  进程的控制终端。对于没有终端的进程，该值为0
*/
typedef struct {
    pid_t pid;
    pid_t ppid;
    char exe_path[256];
    char cmd_line[256];
    char comm[32];
    time_t start_time;
    time_t last_seen;   /* 最后访问时间 */
    int is_container;   /* 是否为容器进程 */
} linx_process_info_t;

typedef struct linx_process_node_s {
    pid_t pid;
    linx_process_info_t *info;
    struct linx_process_node_s *next;
    struct linx_process_node_s *prev;
} linx_process_node_t;

#endif /* __LINX_PROCESS_CACHE_NODE_H__ */