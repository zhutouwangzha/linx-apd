#include "vmlinux/vmlinux.h"

#include "linx_syscall_table.h"

/**
 * 从 /include/uapi/linux/landlock.h 文件拷贝而来
 * 用于支持 landlock_add_rule 系统调用
 */
enum landlock_rule_type {
	LANDLOCK_RULE_PATH_BENEATH = 1,
};

/**
 * 这些宏用于提取参数中的名称
 */
#define EXTRACT_NAMES_FOR_1_ARGS()              
#define EXTRACT_NAMES_FOR_2_ARGS(t, n)          #n
#define EXTRACT_NAMES_FOR_4_ARGS(t, n, ...)     #n, EXTRACT_NAMES_FOR_2_ARGS(__VA_ARGS__)
#define EXTRACT_NAMES_FOR_6_ARGS(t, n, ...)     #n, EXTRACT_NAMES_FOR_4_ARGS(__VA_ARGS__)
#define EXTRACT_NAMES_FOR_8_ARGS(t, n, ...)     #n, EXTRACT_NAMES_FOR_6_ARGS(__VA_ARGS__)
#define EXTRACT_NAMES_FOR_10_ARGS(t, n, ...)    #n, EXTRACT_NAMES_FOR_8_ARGS(__VA_ARGS__)
#define EXTRACT_NAMES_FOR_12_ARGS(t, n, ...)    #n, EXTRACT_NAMES_FOR_10_ARGS(__VA_ARGS__)

#define SELECT_NAME(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, N, ...) EXTRACT_NAMES_FOR_##N##_ARGS

/**
 * 这些宏用于提取参数的类型并转换为linx_type_t
 */
#define GET_PARAM_TYPE(T) _Generic((T){0},                  \
    char:                   LINX_TYPE_INT8,                 \
    short:                  LINX_TYPE_INT16,                \
    int:                    LINX_TYPE_INT32,                \
    long:                   LINX_TYPE_INT64,                \
    long long:              LINX_TYPE_INT64,                \
    unsigned char:          LINX_TYPE_UINT8,                \
    unsigned short:         LINX_TYPE_UINT16,               \
    unsigned int:           LINX_TYPE_UINT32,               \
    unsigned long:          LINX_TYPE_UINT64,               \
    unsigned long long:     LINX_TYPE_UINT64,               \
    char *:                 LINX_TYPE_CHARBUF,              \
    const char *:           LINX_TYPE_CHARBUF,              \
    default:                LINX_TYPE_NONE                  \
)

#define EXTRACT_TYPE_FOR_1_ARGS()
#define EXTRACT_TYPE_FOR_2_ARGS(t, n, ...)  GET_PARAM_TYPE(t)
#define EXTRACT_TYPE_FOR_4_ARGS(t, n, ...)  GET_PARAM_TYPE(t), EXTRACT_TYPE_FOR_2_ARGS(__VA_ARGS__)
#define EXTRACT_TYPE_FOR_6_ARGS(t, n, ...)  GET_PARAM_TYPE(t), EXTRACT_TYPE_FOR_4_ARGS(__VA_ARGS__)
#define EXTRACT_TYPE_FOR_8_ARGS(t, n, ...)  GET_PARAM_TYPE(t), EXTRACT_TYPE_FOR_6_ARGS(__VA_ARGS__)
#define EXTRACT_TYPE_FOR_10_ARGS(t, n, ...) GET_PARAM_TYPE(t), EXTRACT_TYPE_FOR_8_ARGS(__VA_ARGS__)
#define EXTRACT_TYPE_FOR_12_ARGS(t, n, ...) GET_PARAM_TYPE(t), EXTRACT_TYPE_FOR_10_ARGS(__VA_ARGS__)

#define SELECT_TYPE(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, N, ...) EXTRACT_TYPE_FOR_##N##_ARGS

linx_syscall_table_t g_linx_syscall_table[LINX_SYSCALL_MAX_IDX] = {
    #define SYSCALL_MACRO(up_name, low_name, id, e_num, x_num)  \
        [id] = {                                                \
            .enter_num = e_num,                                 \
            .exit_num = x_num,                                  \
            .name = #low_name,                                  \
            .ebpf_prog_name = {                                 \
                [LINX_SYSCALL_TYPE_ENTER] = #low_name"_e",      \
                [LINX_SYSCALL_TYPE_EXIT]  = #low_name"_x"       \
            },
    #define ENTER_PARAM_MACRO(...)                              \
            .param_name = {                                     \
                SELECT_NAME(__VA_ARGS__, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)(__VA_ARGS__) \
            },                                                  \
            .enter_type = {                                     \
                SELECT_TYPE(__VA_ARGS__, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)(__VA_ARGS__) \
            },
    #define EXIT_PARAM_MACRO(...)                               \
            .exit_type = {                                      \
                SELECT_TYPE(__VA_ARGS__, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)(__VA_ARGS__) \
            },                                                  \
        },
    #include "macro/linx_syscalls_macro.h"
};
