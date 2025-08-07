#ifndef __LINX_CONFIG_H__
#define __LINX_CONFIG_H__ 

#include <stdbool.h>
#include <stdint.h>

#include "linx_size_define.h"
#include "linx_syscall_id.h"

typedef struct {
    struct {
        char *output;
        char *log_level;
    } log_config;

    struct {
        char *kind;

        union {
            struct {
                char *path;
            } kmod;

            struct {
                bool drop_mode;
                bool drop_failed;
                uint32_t filter_pids[LINX_BPF_FILTER_PID_MAX_SIZE];
                uint8_t filter_comms[LINX_BPF_FILTER_COMM_MAX_SIZE][LINX_COMM_MAX_SIZE];
                uint8_t interest_syscall_table[LINX_SYSCALL_ID_MAX];
            } ebpf;
        } data;
    } engine;
} linx_global_config_t;

int linx_config_init(void);

void linx_config_deinit(void);

linx_global_config_t *linx_config_get(void);

int linx_config_load(const char *config_file);

int linx_config_reload(void);

#endif /* __LINX_CONFIG_H__ */