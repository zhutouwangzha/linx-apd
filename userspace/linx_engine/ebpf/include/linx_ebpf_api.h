#ifndef __LINX_EBPF_API_H__
#define __LINX_EBPF_API_H__

#include <stdint.h>
#include <stdbool.h>

#include "linx_event.h"

typedef struct {
    struct linx_bpf *skel;
    struct ring_buffer *rb;
    uint8_t ringbuf_data[LINX_EVENT_MAX_SIZE];
} linx_ebpf_t;

int linx_ebpf_init(linx_ebpf_t *bpf_manager);

int linx_ebpf_set_print(void);

int linx_ebpf_open(linx_ebpf_t *bpf_manager);

int linx_ebpf_maps_before_load(linx_ebpf_t *bpf_manager);

int linx_ebpf_load(linx_ebpf_t *bpf_manager);

int linx_ebpf_ringbuf_init(linx_ebpf_t *bpf_manager);

int linx_ebpf_load_tail_call_map(struct linx_bpf *skel, bool *interest_syscall_table);

int linx_ebpf_probe_load(struct linx_bpf *skel);

int linx_ebpf_get_ringbuf_msg(linx_ebpf_t *bpf_manager, linx_event_t **event);

void linx_ebpf_set_boot_time(struct linx_bpf *skel, uint64_t boot_time);

void linx_ebpf_set_snaplen(struct linx_bpf *skel, uint32_t snaplen);

void linx_ebpf_set_do_snaplen(struct linx_bpf *skel, bool do_snaplen);

void linx_ebpf_set_filter_pids(struct linx_bpf *skel, uint32_t *filter_pids);

void linx_ebpf_set_filter_comms(struct linx_bpf *skel, uint8_t filter_comms[][16]);

void linx_ebpf_set_drop_mode(struct linx_bpf *skel, bool value);

void linx_ebpf_set_drop_failed(struct linx_bpf *skel, bool value);

void linx_ebpf_set_interesting_syscalls_table(struct linx_bpf *skel, bool *interest_syscall_table);

void linx_ebpf_set_event_num_params(struct linx_bpf *skel);

#endif /* __LINX_EBPF_API_H__ */
