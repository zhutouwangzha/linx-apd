#include <unistd.h>

#include "linx_log.h"
#include "linx_ebpf_common.h"
#include "linx_ebpf_api.h"
#include "linx_event_table.h"
#include "linx_config.h"
#include "linx_exit_extra_id.h"
#include "struct_define.h"

// static const char *syscall_exit_extra_names[LINX_EXIT_EXTRA_ID_MAX] = {
//     [T1_EXECV_X] = "t1_execve_x",
// };

int linx_ebpf_maps_before_load(linx_ebpf_t *bpf_manager)
{
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpu == -1) {
        LINX_LOG_ERROR("Failed to get the number of system cores!");
        return -1;
    }

	if(bpf_map__set_max_entries(bpf_manager->skel->maps.linx_ringbuf_maps, ncpu)) {
		LINX_LOG_ERROR("unable to set max entries(%d) for 'linx_ringbuf_maps'!",
                       ncpu);
		return -1;
	}

    return 0;
}

int linx_ebpf_get_capture_set(struct linx_bpf *skel, linx_capture_set_t *set)
{
    int ret;
    uint32_t key = 0;
    int fd = bpf_map__fd(skel->maps.g_capture_set);
    if (fd <= 0) {
        LINX_LOG_ERROR("Unable to get g_capture_set map fd!");
        return -1;
    }

    if ((ret = bpf_map_lookup_elem(fd, &key, set)) != 0) {
        LINX_LOG_ERROR("Unable to get g_capture_set!");
    }

    return ret;
}

int linx_ebpf_update_capture_set(struct linx_bpf *skel, linx_capture_set_t *set)
{
    int ret;
    uint32_t key = 0;
    int fd = bpf_map__fd(skel->maps.g_capture_set);
    if (fd <= 0) {
        LINX_LOG_ERROR("Unable to get g_capture_set map fd!");
        return -1;
    }

    if ((ret = bpf_map_update_elem(fd, &key, set, BPF_ANY)) != 0) {
        LINX_LOG_ERROR("Unable to update g_capture_set!");
    }

    return ret;
}

void linx_ebpf_set_snaplen(struct linx_bpf *skel, uint32_t snaplen)
{
    linx_capture_set_t set;
    if (linx_ebpf_get_capture_set(skel, &set) != 0) {
        return;
    }

    if (snaplen > LINX_SNAPLEN_MAX) {
        LINX_LOG_WARNING("snaplen can't over %d, set snaplen to %d\n", 
                         LINX_SNAPLEN_MAX, LINX_DEFAULT_SNAPLEN);
        snaplen = LINX_DEFAULT_SNAPLEN;
    }

    set.snaplen = snaplen;

    linx_ebpf_update_capture_set(skel, &set);
}

void linx_ebpf_set_do_snaplen(struct linx_bpf *skel, bool do_snaplen)
{
    linx_capture_set_t set;
    if (linx_ebpf_get_capture_set(skel, &set) != 0) {
        return;
    }

    set.do_snaplen = do_snaplen;

    linx_ebpf_update_capture_set(skel, &set);
}

void linx_ebpf_set_boot_time(struct linx_bpf *skel, uint64_t boot_time)
{
    linx_capture_set_t set;
    if (linx_ebpf_get_capture_set(skel, &set) != 0) {
        return;
    }

    set.boot_time = boot_time;

    linx_ebpf_update_capture_set(skel, &set);
}

void linx_ebpf_set_drop_mode(struct linx_bpf *skel, bool value)
{
    linx_capture_set_t set;
    if (linx_ebpf_get_capture_set(skel, &set) != 0) {
        return;
    }

    set.drop_mode = value;

    linx_ebpf_update_capture_set(skel, &set);
}

void linx_ebpf_set_drop_failed(struct linx_bpf *skel, bool value)
{
    linx_capture_set_t set;
    if (linx_ebpf_get_capture_set(skel, &set) != 0) {
        return;
    }

    set.drop_failed = value;

    linx_ebpf_update_capture_set(skel, &set);
}

void linx_ebpf_set_filter_pids(struct linx_bpf *skel, uint32_t *filter_pids)
{
    for (int i = 0; 
         i < LINX_BPF_FILTER_PID_MAX_SIZE &&
         filter_pids[i]; 
         ++i)
    {
        skel->bss->g_filter_pids[i] = filter_pids[i];
    }
}

void linx_ebpf_set_filter_comms(struct linx_bpf *skel, uint8_t filter_comms[][16])
{
    int byte_write, buf_size = LINX_COMM_MAX_SIZE;

    for (int i = 0;
         i < LINX_BPF_FILTER_COMM_MAX_SIZE &&
         filter_comms[i][0] != 0;
         ++i)
    {
        byte_write = snprintf((char *)skel->bss->g_filter_comms[i], buf_size, "%s", 
                              (char *)filter_comms[i]);
        if (byte_write < 0) {
            LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf for %d time!", 
                             strlen((char *)filter_comms[i]), 
                             (char *)filter_comms[i], 
                             buf_size, 
                             i);
        }
    }
}

void linx_ebpf_set_interesting_syscalls_table(struct linx_bpf *skel, bool *interest_syscall_table)
{
    for (int i = 0; i < LINX_SYSCALL_ID_MAX; ++i) {
        skel->bss->g_interesting_syscalls_table[i] =
            interest_syscall_table[i];
    }
}

static int linx_ebpf_add_prog_to_tail_table(struct linx_bpf *skel, int tail_tabld_fd,
                                            const char *prog_name, int key)
{
    struct bpf_program *bpf_prog = NULL;
    int bpf_prog_fd = 0;

    bpf_prog = bpf_object__find_program_by_name(skel->obj, prog_name);
    if (!bpf_prog) {
        LINX_LOG_WARNING("unable to find BPF program '%s'!", prog_name);
        return 0;
    }

    bpf_prog_fd = bpf_program__fd(bpf_prog);
    if (bpf_prog_fd <= 0) {
        LINX_LOG_ERROR("unable to get the fd for BPF program '%s'!", prog_name);
        goto clean_add_prog_to_tail_table;
    }

    if (bpf_map_update_elem(tail_tabld_fd, &key, &bpf_prog_fd, BPF_ANY)) {
        LINX_LOG_ERROR("unable to update the tail table with BPF program '%s'!", prog_name);
        goto clean_add_prog_to_tail_table;
    }
    return 0;

clean_add_prog_to_tail_table:
    close(bpf_prog_fd);
    return -1;
}

int linx_ebpf_load_tail_call_map(struct linx_bpf *skel, bool *interest_syscall_table)
{
    int enter_table_fd = bpf_map__fd(skel->maps.syscall_enter_tail_table);
    int exit_table_fd = bpf_map__fd(skel->maps.syscall_exit_tail_table);
    char prog_name[64];

    for (int syscall_id = 0; syscall_id < LINX_SYSCALL_ID_MAX; ++syscall_id) {
        if (!interest_syscall_table[syscall_id]) {
            LINX_LOG_DEBUG("The %s(%d) syscall is ont interest!",
                           g_linx_event_table[syscall_id * 2].name, syscall_id);
            continue;
        }

        snprintf(prog_name, sizeof(prog_name), "%s_e", 
                g_linx_event_table[syscall_id * 2].name);

        if (linx_ebpf_add_prog_to_tail_table(skel, enter_table_fd, prog_name, syscall_id)) {
            LINX_LOG_ERROR("Failed to add '%s' prog to syscall_enter_tail_table!", 
                           prog_name);
            goto clean_load_tail_call_map;
        }

        snprintf(prog_name, sizeof(prog_name), "%s_x", 
                g_linx_event_table[syscall_id * 2].name);

        if (linx_ebpf_add_prog_to_tail_table(skel, exit_table_fd, prog_name, syscall_id)) {
            LINX_LOG_ERROR("Failed to add '%s' prog to syscall_exit_tail_table!", 
                           prog_name);
            goto clean_load_tail_call_map;
        }
    }

    return 0;

clean_load_tail_call_map:
    close(enter_table_fd);
    close(exit_table_fd);
    return -1;
}

int linx_ebpf_final_maps_after_load(struct linx_bpf *skel)
{
    int ret;
    linx_global_config_t *config = linx_config_get();
    if (config == NULL) {
        return -1;
    }

    linx_capture_set_t set = {0};
    ret = linx_ebpf_update_capture_set(skel, &set);
    if (ret) {
        return ret;
    }

    linx_ebpf_set_filter_pids(skel, config->engine.data.ebpf.filter_pids);

    linx_ebpf_set_filter_comms(skel, config->engine.data.ebpf.filter_comms);

    linx_ebpf_set_drop_mode(skel, config->engine.data.ebpf.drop_mode);

    linx_ebpf_set_drop_failed(skel, config->engine.data.ebpf.drop_failed);

    linx_ebpf_set_snaplen(skel, config->snaplen);

    linx_ebpf_set_do_snaplen(skel, false);

    linx_ebpf_set_interesting_syscalls_table(skel, config->engine.data.ebpf.interest_syscall_table);

    return ret;
}
