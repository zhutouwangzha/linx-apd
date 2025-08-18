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
 * ebpf 的整体配置
 */
struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, uint32_t);
	__type(value, linx_capture_set_t);
} g_capture_set __weak SEC(".maps");

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
