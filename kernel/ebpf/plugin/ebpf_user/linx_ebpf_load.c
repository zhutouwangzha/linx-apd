#include <time.h>
#include <net/if.h>

#include "linx_ebpf_common.h"
#include "linx_ebpf_api.h"
#include "linx_log_api.h"
#include "linx_syscall_table.h"
#include "plugin_config.h"

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
    LINX_LOG_DEBUG_V(format, args);
    return 0;
}

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

int linx_ebpf_set_print(void)
{
    libbpf_set_print(libbpf_print_fn);
    return 0;
}

int linx_ebpf_open(linx_ebpf_t *bpf_manager)
{
    bpf_manager->skel = linx_bpf__open();
    if (!bpf_manager->skel) {
        LINX_LOG_ERROR("Failed to open BPF skeleton!");
        return -1;
    }

    return 0;
}

int linx_ebpf_load(linx_ebpf_t *bpf_manager)
{
    for (int i = 0; i < LINX_SYSCALL_MAX_IDX; ++i) {
        if (g_plugin_config.open_config.interest_syscall_table[i]) {
            LINX_LOG_DEBUG("The %s(%d) syscall is ont interest!",
                           g_linx_syscall_table[i].name, i);
            continue;
        }

        for (int j = 0; j < 2; ++j) {
            if (!g_linx_syscall_table[i].ebpf_prog_name[j]) {
                LINX_LOG_DEBUG("The BPF function for to %s the %s syscall is undefined!",
                               j ? "enter" : "exit", g_linx_syscall_table[i].name);
                continue;
            }

            struct bpf_program *p =
            bpf_object__find_program_by_name(bpf_manager->skel->obj, 
                                             g_linx_syscall_table[i].ebpf_prog_name[j]);
            if (!p) {
                LINX_LOG_DEBUG("The BPF function for to %s the %s syscall does not exist in skel!",
                               j ? "enter" : "exit", g_linx_syscall_table[i].name);
                continue;
            }

            bpf_program__set_autoload(p, false);
        }
    }

    if (linx_bpf__load(bpf_manager->skel)) {
        LINX_LOG_ERROR("Failed to load BPF skeleton!\n");
        linx_bpf__destroy(bpf_manager->skel);
        return -1;
    }

    return 0;
}

int linx_ebpf_probe_load(struct linx_bpf *skel)
{
    if (!bpf_program__attach(skel->progs.sys_enter)) {
        LINX_LOG_ERROR("Failed to attach sys_enter\n");
        return -1;
    }

    if (!bpf_program__attach(skel->progs.sys_exit)) {
        LINX_LOG_ERROR("Failed to attach sys_exit\n");
        return -1;
    }

    skel->links.xdp_pass = bpf_program__attach_xdp(skel->progs.xdp_pass, if_nametoindex("ens3"));
    if (!skel->links.xdp_pass) {
        LINX_LOG_ERROR("Failed to attach xdp_pass\n");
    }

    return 0;
}

int linx_ebpf_init(linx_ebpf_t *bpf_manager)
{
    int ret = 0;
    uint64_t boot_time = 0;

    ret = ret ? : linx_ebpf_set_print();
    ret = ret ? : linx_ebpf_open(bpf_manager);
    ret = ret ? : linx_ebpf_maps_before_load(bpf_manager);
    ret = ret ? : linx_ebpf_load(bpf_manager);
    ret = ret ? : linx_ebpf_ringbuf_init(bpf_manager);
    ret = ret ? : linx_ebpf_load_tail_call_map(bpf_manager->skel);
    ret = ret ? : linx_ebpf_probe_load(bpf_manager->skel);
    ret = ret ? : linx_get_boot_time(&boot_time);

    linx_ebpf_set_boot_time(bpf_manager->skel, boot_time);

    linx_ebpf_set_filter_pids(bpf_manager->skel);

    linx_ebpf_set_filter_comms(bpf_manager->skel);

    linx_ebpf_set_drop_mode(bpf_manager->skel, g_plugin_config.open_config.drop_mode);

    linx_ebpf_set_drop_failed(bpf_manager->skel, g_plugin_config.open_config.drop_failed);

    linx_ebpf_set_interesting_syscalls_table(bpf_manager->skel);

    return ret;
}
