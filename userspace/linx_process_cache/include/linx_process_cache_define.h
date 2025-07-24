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

/**
 * 启用事件驱动模式
 * 当启用时，缓存将主要依赖系统调用事件来更新进程信息
 * 当禁用时，缓存将主要依赖定期扫描/proc目录
*/
#define LINX_PROCESS_CACHE_EVENT_DRIVEN_ENABLED 1

/**
 * 事件驱动模式下的轮询间隔（秒）
 * 即使在事件驱动模式下，仍然需要定期扫描以捕获遗漏的进程
*/
#define LINX_PROCESS_CACHE_EVENT_DRIVEN_POLL_INTERVAL 10

/**
 * 短暂进程的缓存保留时间（秒）
 * 对于生命周期很短的进程，我们需要保留其信息一段时间
*/
#define LINX_PROCESS_CACHE_SHORT_LIVED_RETAIN_TIME 5

#endif /* __LINX_PROCESS_CACHE_DEFINE_H__ */
