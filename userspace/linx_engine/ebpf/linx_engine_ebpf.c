#include <time.h>

#include "linx_log.h"
#include "linx_ebpf_api.h"
#include "linx_engine_ebpf.h"

linx_ebpf_t s_bpf_manager = {0};

static inline uint64_t timespec_to_nsec(const struct timespec* ts) {
	return ts->tv_sec * 1000000000 + ts->tv_nsec;
}

static int linx_get_boot_time(uint64_t* boot_time) {
	struct timespec wall_ts, boot_ts;

	if(clock_gettime(CLOCK_BOOTTIME, &boot_ts) < 0) {
        LINX_LOG_ERROR("Failed to get CLOCK_BOOTTIME!");
		return -1;
	}

	if(clock_gettime(CLOCK_REALTIME, &wall_ts) < 0) {
        LINX_LOG_ERROR("Failed to get CLOCK_REALTIME!");
		return -1;
	}

	*boot_time = timespec_to_nsec(&wall_ts) - timespec_to_nsec(&boot_ts);

	return 0;
}

int ebpf_init(void)
{
    int ret = 0;
    uint64_t boot_time = 0;

    ret = ret ? : linx_ebpf_set_print();
    ret = ret ? : linx_ebpf_open(&s_bpf_manager);
    ret = ret ? : linx_ebpf_maps_before_load(&s_bpf_manager);
    ret = ret ? : linx_ebpf_load(&s_bpf_manager);
    ret = ret ? : linx_ebpf_ringbuf_init(&s_bpf_manager);
    ret = ret ? : linx_ebpf_load_tail_call_map(s_bpf_manager.skel);
    ret = ret ? : linx_ebpf_probe_load(s_bpf_manager.skel);
    ret = ret ? : linx_get_boot_time(&boot_time);

    linx_ebpf_set_boot_time(s_bpf_manager.skel, boot_time);

    linx_ebpf_set_filter_pids(s_bpf_manager.skel);

    linx_ebpf_set_filter_comms(s_bpf_manager.skel);

    linx_ebpf_set_drop_mode(s_bpf_manager.skel, 0);

    linx_ebpf_set_drop_failed(s_bpf_manager.skel, 0);

    linx_ebpf_set_interesting_syscalls_table(s_bpf_manager.skel);

    return ret;
}

int ebpf_start(void)
{
    return 0;
}

int ebpf_stop(void)
{
    return 0;
}

int ebpf_next(linx_event_t **event)
{
    return linx_ebpf_get_ringbuf_msg(&s_bpf_manager, event);
}

int ebpf_close(void)
{
    return 0;
}

linx_engine_vtable_t ebpf_vtable = {
    .name = "ebpf",
    .init = ebpf_init,
    .start = ebpf_start,
    .stop = ebpf_stop,
    .next = ebpf_next,
    .close = ebpf_close
};
