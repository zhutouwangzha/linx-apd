#ifndef __LINX_PROCESS_CACHE_DEFINE_H__
#define __LINX_PROCESS_CACHE_DEFINE_H__ 

/**
 * 缓存更新间隔秒数（常规模式）
*/
#define LINX_PROCESS_CACHE_UPDATE_INTERVAL 1

/**
 * 高频扫描间隔（毫秒）
 * 用于捕获短暂进程
*/
#define LINX_PROCESS_CACHE_HIGH_FREQ_INTERVAL_MS 10

/**
 * 是否启用高频扫描模式
*/
#define LINX_PROCESS_CACHE_HIGH_FREQ_ENABLED 1

/**
 * 高频扫描持续时间（秒）
 * 启动后持续高频扫描一段时间，然后降低频率
*/
#define LINX_PROCESS_CACHE_HIGH_FREQ_DURATION 60

/**
 * 缓存过期时间秒数
*/
#define LINX_PROCESS_CACHE_EXPIRE_TIME 20

/**
 * 短暂进程保留时间（秒）
 * 对于生命周期很短的进程，保留更长时间
*/
#define LINX_PROCESS_CACHE_SHORT_LIVED_RETAIN_TIME 30

/**
 * 缓存线程数
*/
#define LINX_PROCESS_CACHE_THREAD_NUM 8

/**
 * 快速扫描线程数
*/
#define LINX_PROCESS_CACHE_FAST_SCAN_THREADS 2

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

/**
 * 进程信息缓存的初始哈希表大小
*/
#define LINX_PROCESS_CACHE_INITIAL_SIZE 1024

/**
 * 批量处理大小
*/
#define LINX_PROCESS_CACHE_BATCH_SIZE 100

#endif /* __LINX_PROCESS_CACHE_DEFINE_H__ */
