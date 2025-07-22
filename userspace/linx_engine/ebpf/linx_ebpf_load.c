#include "linx_log.h"
#include "linx_ebpf_api.h"
#include "linx_ebpf_common.h"
#include "linx_syscall_table.h"

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
    LINX_LOG_DEBUG_V(format, args);
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
        // if (g_plugin_config.open_config.interest_syscall_table[i]) {
        //     LINX_LOG_DEBUG("The %s(%d) syscall is ont interest!",
        //                    g_linx_syscall_table[i].name, i);
        //     continue;
        // }
        if (i == LINX_SYSCALL_UNLINK || 
            i == LINX_SYSCALL_UNLINKAT)
        {
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

    // skel->links.xdp_pass = bpf_program__attach_xdp(skel->progs.xdp_pass, if_nametoindex("ens3"));
    // if (!skel->links.xdp_pass) {
    //     LINX_LOG_ERROR("Failed to attach xdp_pass\n");
    // }

    return 0;
}

