#ifndef __LINX_SYSCALL_TABLE_H__
#define __LINX_SYSCALL_TABLE_H__

#include "linx_type.h"
#include "linx_syscall_id.h"
#include "linx_size_define.h"

typedef struct {
    uint32_t    enter_num;                              /* 系统调用入参的个数 */
    uint32_t    exit_num;                               /* 系统调用出参的个数 */
    const char *name;                                   /* 系统调用名 */
    const char *ebpf_prog_name[LINX_SYSCALL_TYPE_MAX];  /* 在ebpf内核程序中插桩点函数名,0是进入,1是退出 */
    const char *param_name[SYSCALL_PARAMS_MAX_COUNT];   /* 各个参数的名称 */
    linx_type_t enter_type[SYSCALL_PARAMS_MAX_COUNT];   /* 入参的类型 */
    linx_type_t exit_type[1];                           /* 出参的类型 */
} linx_syscall_table_t;

extern linx_syscall_table_t g_linx_syscall_table[LINX_SYSCALL_MAX_IDX];

#endif /* __LINX_SYSCALL_TABLE_H__ */
