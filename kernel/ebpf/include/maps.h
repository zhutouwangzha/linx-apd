#ifndef __MAPS_H__
#define __MAPS_H__

#include "bpf_common.h"
#include "struct_define.h"
#include "linx_syscall_id.h"
#include "linx_exit_extra_id.h"

/**
 * 需要过滤掉的pid集合
 */
__weak uint32_t g_filter_pids[LINX_BPF_FILTER_PID_MAX_SIZE];

/**
 * 需要过滤掉的任务集合
 */
__weak char g_filter_comms[LINX_BPF_FILTER_COMM_MAX_SIZE][LINX_COMM_MAX_SIZE];

/**
 * 表示需要采集哪些系统调用
 */
__weak uint8_t g_interesting_syscalls_table[LINX_SYSCALL_ID_MAX];

/**
 * 应用层获取到的启动时间
 * 该时间+bpf中获取的时间=系统时间
 */
__weak uint64_t g_boot_time;

/**
 * 丢弃模式的总体控制开关
 * 为1时，放弃采集所有的系统调用
 */
__weak uint8_t g_drop_mode;

/**
 * 是否放弃采集失败的系统调用
 * 为1时，放弃采集
 */
__weak uint8_t g_drop_failed;

/**
 * 系统调用退出的尾部调用表
 */
struct {
	__uint(type, BPF_MAP_TYPE_PROG_ARRAY);
	__uint(max_entries, LINX_SYSCALL_ID_MAX);
	__type(key, uint32_t);
	__type(value, uint32_t);
} syscall_enter_tail_table __weak SEC(".maps");

/**
 * 系统调用退出的尾部调用表
 */
struct {
	__uint(type, BPF_MAP_TYPE_PROG_ARRAY);
	__uint(max_entries, LINX_SYSCALL_ID_MAX);
	__type(key, uint32_t);
	__type(value, uint32_t);
} syscall_exit_tail_table __weak SEC(".maps");

/**
 * 需要多个处理的系统调用
*/
struct {
	__uint(type, BPF_MAP_TYPE_PROG_ARRAY);
	__uint(max_entries, LINX_EXIT_EXTRA_ID_MAX);
	__type(key, uint32_t);
	__type(value, uint32_t);
} syscall_exit_extra_tail_table __weak SEC(".maps");

/**
 * 消息交互的环形缓冲区
 */
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, LINX_EVENT_MAX_SIZE);
} ringbuf_map __weak SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__type(key, uint32_t);
	__type(value, linx_ringbuf_t);
} linx_ringbuf_maps __weak SEC(".maps");

#endif /* __MAPS_H__ */
