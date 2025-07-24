#ifndef __LINX_PROCESS_CACHE_DEFINE_H__
#define __LINX_PROCESS_CACHE_DEFINE_H__ 

/**
 * 缓存更新间隔秒数
*/
#define LINX_PROCESS_CACHE_UPDATE_INTERVAL 1

/**
 * 缓存过期时间秒数
*/
#define LINX_PROCESS_CACHE_EXPIRE_TIME 20

/**
 * 缓存线程数
*/
#define LINX_PROCESS_CACHE_THREAD_NUM 4

/**
 * proc 路径最大长度
*/
#define PROC_PATH_MAX_LEN 256

/**
 * proc cmdline 最大长度
*/
#define PROC_CMDLINE_LEN  4096

/**
 * proc comm 最大长度
*/
#define PROC_COMM_MAX_LEN 256

#endif /* __LINX_PROCESS_CACHE_DEFINE_H__ */
