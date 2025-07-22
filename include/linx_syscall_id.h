#ifndef __LINX_SYSCALL_ID_H__
#define __LINX_SYSCALL_ID_H__

typedef enum {
    #define SYSCALL_MACRO(up_name, low_name, id, ...)  \
        LINX_SYSCALL_##up_name = id,
    #define ENTER_PARAM_MACRO(...)
    #define EXIT_PARAM_MACRO(...)
    #include "macro/linx_syscalls_macro.h"
    LINX_SYSCALL_MAX_IDX
} linx_syscall_id_t;

#endif /* __LINX_SYSCALL_ID_H__ */
