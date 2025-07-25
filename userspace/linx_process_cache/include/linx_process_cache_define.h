#ifndef __LINX_PROCESS_CACHE_DEFINE_H__
#define __LINX_PROCESS_CACHE_DEFINE_H__ 

/**
 * 缓存更新间隔秒数 (改为毫秒级别以提高短生命周期进程的捕获能力)
*/
#define LINX_PROCESS_CACHE_UPDATE_INTERVAL_MS 100

/**
 * 缓存过期时间秒数
*/
#define LINX_PROCESS_CACHE_EXPIRE_TIME 20

/**
 * 短生命周期进程退出后的缓存保留时间（秒）
 * 用于确保快速执行的进程（如find命令）信息能被规则引擎获取
*/
#define LINX_PROCESS_CACHE_SHORT_LIVED_RETAIN_TIME 5

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
