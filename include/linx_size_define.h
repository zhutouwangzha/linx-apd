#ifndef __LINX_SIZE_DEFINE_H__
#define __LINX_SIZE_DEFINE_H__

/**
 * 系统调用参数的最大个数
 */
#define SYSCALL_PARAMS_MAX_COUNT    (32)

/**
 * 事件头的大小
 */
#define LINX_EVENT_HEADER_SIZE      (sizeof(linx_event_t))

/**
 * 一次事件所获取的最大长度
 */
#define LINX_EVENT_MAX_SIZE         (32 * 1024)

/**
 * 真正的数据长度
 */
#define LINX_EVENT_DATA_SIZE        (LINX_EVENT_MAX_SIZE - LINX_EVENT_HEADER_SIZE)

/**
 * 命令名称的最大长度
 */
#define LINX_COMM_MAX_SIZE          (16)

/**
 * 执行的命令的最大长度
 */
#define LINX_CMDLINE_MAX_SIZE       (128)

/**
 * 获取当前任务关联的fd的最大值
 */
#define LINX_FDS_MAX_SIZE           (12)

/**
 * 文件对应路径的最大长度
 */
#define LINX_PATH_MAX_SIZE          (256)

/**
 * 拷贝路径字符串时的最大长度
 */
#define LINX_CHARBUF_MAX_SIZE       (4096)

/**
 * 可过滤PID的最大数量
 */
#define LINX_BPF_FILTER_PID_MAX_SIZE    (64)

/**
 * 可过滤COMM的最大数量
 */
#define LINX_BPF_FILTER_COMM_MAX_SIZE   (16)

/**
 * 网络相关的长度定义
*/
#define LINX_FAMILY_SIZE                sizeof(uint8_t)
#define LINX_IPV4_SIZE                  sizeof(uint32_t)
#define LINX_IPV6_SIZE                  16
#define LINX_PORT_SIZE                  sizeof(uint16_t)

#endif /* __LINX_SIZE_DEFINE_H__ */
