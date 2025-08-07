#include "linx_log.h"
#include "linx_ebpf_api.h"
#include "linx_ebpf_common.h"
#include "linx_event_table.h"
#include "linx_config.h"
static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
    (void)level;

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
    linx_global_config_t *config = linx_config_get();

    for (int i = 0; i < LINX_SYSCALL_ID_MAX; ++i) {
        if (config->engine.data.ebpf.interest_syscall_table[i]) {
            LINX_LOG_DEBUG("The %s(%d) syscall is ont interest!",
                           g_linx_event_table[i * 2].name, i);
            continue;
        }

        for (int j = 0; j < 2; ++j) {
            char bpf_program_name[64];
            if (j) {
                snprintf(bpf_program_name, sizeof(bpf_program_name), 
                         "%s_x", g_linx_event_table[i * 2].name);
            } else {
                snprintf(bpf_program_name, sizeof(bpf_program_name), 
                         "%s_e", g_linx_event_table[i * 2].name);
            }

            struct bpf_program *p =
            bpf_object__find_program_by_name(bpf_manager->skel->obj, 
                                             bpf_program_name);
            if (!p) {
                LINX_LOG_DEBUG("The BPF function for to %s the %s syscall does not exist in skel!",
                               j ? "enter" : "exit", bpf_program_name);
                continue;
            }

            if (bpf_program__set_autoload(p, false) == 0) {
                LINX_LOG_DEBUG("Disable BPF program '%s' success", bpf_program_name);
            } else {
                LINX_LOG_DEBUG("Disable BPF program '%s' failed", bpf_program_name);
            }
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

