#ifndef __LINX_EVENTS_PUBLIC_H__
#define __LINX_EVENTS_PUBLIC_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#include "linx_event_type.h"
#include "linx_event_table.h"

#define _packed __attribute__((packed))

/* 数据长度定义 */
#define SIZE_32             (32)
#define SIZE_64             (64)
#define SIZE_512            (512)
#define SIZE_1024           (1024)
#define MAX_CMDLINE_LEN     (128)


#define PROC_SIZE       SIZE_32
#define PP_PROC_SIZE    SIZE_32
#define FD_NAME_SIZE    SIZE_64
#define FD_NUM          (12)

/* 操作标志定义 */
#ifndef ENABLE
#define ENABLE      1
#define DISABLE     0
#endif

#define SYSCALL_ENTER   0   /* 系统调用进入 */
#define SYSCALL_EXIT    1   /* 系统调用退出 */


#define BUFFER_SIZE     (8 * 1024 * 1024)   // 每个CPU 8MB缓冲区

/**********************************************************************
 * 输出信息结构
***********************************************************************/

typedef struct kmod_thread_info
{
    char fd_name[FD_NUM][FD_NAME_SIZE];           /* 线程文件描述符 */
}kmod_thread_info_t;


// 事件头结构
// typedef struct event_header
// {
//     uint64_t ts;            /* 时间戳 */
//     uint32_t type;          /* 事件类型 */
//     uint32_t evt_type;      /* 具体事件类型 */
//     uint32_t syscall_nr;    /* 系统调用号 */
//     uint32_t pid;           /* 进程ID */
//     uint32_t tid;           /* 线程ID */
//     uint32_t uid;           /* 用户ID */
//     uint32_t gid;           /* 组ID */
//     uint32_t len;           /* 事件总长度  */
//     uint32_t ppid;          /* 父进程ID */
//     uint32_t nparams;       /* 参数数目 */
//     uint64_t params_size[32]; /* 参数长度 */
//     int64_t  res;           /* 返回值 */
//     int32_t  cpu;           /*cpu */


//     char proc[PROC_SIZE];           /* 操作命令 */
//     char pp_proc[PP_PROC_SIZE];     /* 父进程操作命令 */
//     char cmdline[MAX_CMDLINE_LEN];  /* 操作完整命令 */
//     int fd_val[FD_NUM];             /* 文件操作描述符 */
//     uint8_t args[0];                /* 参数数据 */
// }event_header_t;

/**********************************************************************
 * 环形缓冲区信息结构
***********************************************************************/
struct kmod_ringbuffer_info
{
    uint32_t head;          // 生产者位置
    uint32_t tail;          // 消费者者位置
    uint64_t n_evts;        // 收集的事件数量
    uint64_t n_drop_evts;   // 丢弃的事件数量
};


/**********************************************************************
 * 操作标志定义区
***********************************************************************/
/* IOCTL操作命令 */
#define SYSMON_IOCTL_MAGIC  'l'
#define SYSMON_IOCTL_DISABLE_DROPPING_MODE      _IO(SYSMON_IOCTL_MAGIC, 0)
#define SYSMON_IOCTL_ENABLE_DROPPING_MODE       _IO(SYSMON_IOCTL_MAGIC, 1)
#define SYSMON_IOCTL_GET_VTID                   _IO(SYSMON_IOCTL_MAGIC, 2)
#define SYSMON_IOCTL_GTE_PID                    _IO(SYSMON_IOCTL_MAGIC, 3)
#define SYSMON_IOCTL_GTE_CURRENT_TID            _IO(SYSMON_IOCTL_MAGIC, 4)
#define SYSMON_IOCTL_GTE_CURRENT_PID            _IO(SYSMON_IOCTL_MAGIC, 5)
#define SYSMON_IOCTL_ENABLE_SYSCALL             _IO(SYSMON_IOCTL_MAGIC, 6)
#define SYSMON_IOCTL_DISABLE_SYSCALL            _IO(SYSMON_IOCTL_MAGIC, 7)
#define SYSMON_IOCTL_ENABLE_TP                  _IO(SYSMON_IOCTL_MAGIC, 8)
#define SYSMON_IOCTL_DISABLE_TP                 _IO(SYSMON_IOCTL_MAGIC, 9)
#define SYSMON_IOCTL_ENABLE_REAL_ARGS           _IO(SYSMON_IOCTL_MAGIC, 10)  // 使能获取真实参数信息
#define SYSMON_IOCTL_DISABLE_REAL_ARGS          _IO(SYSMON_IOCTL_MAGIC, 11)
#define SYSMON_IOCTL_ENABLE_FDS                 _IO(SYSMON_IOCTL_MAGIC, 12)  // 使能获取文件描述符
#define SYSMON_IOCTL_DISABLE_FDS                _IO(SYSMON_IOCTL_MAGIC, 13)


/* used flag*/
#define SC_UF_NONE           (0)
#define SC_UF_USED           (1 << 1)
#define SC_UF_NEVER_DROP     (1 << 2)
#define SC_UF_ALWAYS_DROP    (1 << 3)

/*
 * File flags
 */
#define SYSMON_O_NONE          0
#define SYSMON_O_RDONLY        (1 << 0)                    
#define SYSMON_O_WRONLY        (1 << 1)                    
#define SYSMON_O_RDWR          (PPM_O_RDONLY | PPM_O_WRONLY) 
#define SYSMON_O_CREAT         (1 << 2)                     
#define SYSMON_O_APPEND        (1 << 3) 
#define SYSMON_O_DSYNC         (1 << 4)
#define SYSMON_O_EXCL          (1 << 5)
#define SYSMON_O_NONBLOCK      (1 << 6)
#define SYSMON_O_SYNC          (1 << 7)
#define SYSMON_O_TRUNC         (1 << 8)
#define SYSMON_O_DIRECT        (1 << 9)
#define SYSMON_O_DIRECTORY     (1 << 10)
#define SYSMON_O_LARGEFILE     (1 << 11)
#define SYSMON_O_CLOEXEC       (1 << 12)
#define SYSMON_O_TMPFILE       (1 << 13)
#define SYSMON_O_F_CREATED     (1 << 14)   /* 文件在系统调用间创建 */
#define SYSMON_FD_UPPER_LAYER  (1 << 15)   /* 文件来自上层 */
#define SYSMON_FD_LOWER_LAYER  (1 << 16)   /* 文件来自下层 */

/* 错误标志 */
#define ERR_BASE            (0x0)
#define SYSMON_SUCCESS      (0x0)
#define ERR_BUFFER_FULL     (0x1)
#define ERR_INVALID_MEMORY  (0x2)
#define ERR_OTHER           (0x3)

/**********************************************************************
 * 系统调用具体参数信息数据结构
***********************************************************************/
struct event_filler_arguments;

typedef int (*filler_callback_t)(struct event_filler_arguments *args);
/* 实际参数回调数据结构 */
struct sysmon_event_entry {
	filler_callback_t filler_callback;
};

// typedef enum {
//     SYSMON_GENERIC_E = 0,
// 	SYSMON_GENERIC_X = 1,
// 	SYSMON_SYSCALL_SETREGID_X = 429,
//     SYSMON_SYSCALL_MAX,
// } sysmon_event_code;


// enum sysmon_param_type {
// 	PT_NONE = 0,
// 	PT_INT8 = 1,
// 	PT_INT16 = 2,
// 	PT_INT32 = 3,
//     PT_INT64 = 4,
// 	PT_UINT8 = 5,
// 	PT_UINT16 = 6,
// 	PT_UINT32 = 7,
// 	PT_UINT64 = 8,
// 	PT_CHARBUF = 9,
//     PT_BYTEBUF = 10,
//     PT_ERRNO = 11,
//     PT_SOCKADDR = 12,
//     PT_SOCKTUPLE = 13,
//     PT_SIGTYPE = 14,
//     PT_PORT = 15,
//     PT_IPV4ADDR= 16,
//     PT_INT64LIST = 17,
//     PT_MAX,     
// };

enum sysmon_print_type {
	PF_NA = 0,
	PF_DEC = 1,     /* decimal */
	PF_HEX = 2,     /* hexadecimal */
	PF_OCT = 3,     /* octal */
};



// struct sysmon_param_info {
// 	char name[32];                  /* 参数名称 */
// 	enum sysmon_param_type type;    /* 数据类型：如uint32_t、uint16_t */
//     enum sysmon_print_type fmt;    /* 打印类型：如十进制、八进制、十六进制 */
// };

struct syscall_evt_pair {
	int flags;                              /*  操作标志：系统调用启用 |  丢弃模式标志 */
	linx_event_type_t enter_event_type;     /* 系统调用进入 */
	linx_event_type_t exit_event_type;
} _packed;

// struct sysmon_event_info
// {
//     char name[32];
//     uint32_t nparams;
//     struct sysmon_param_info params[8];
// };

typedef enum {
	SYSMON_TP_SYS_ENTER = 0,
	SYSMON_TP_SYS_EXIT = 1,
    SYSMON_TP_SYS_SCHED_SWITCH = 2,
    SYSMON_TP_MAX,
}sysmon_tp_code;


/**********************************************************************
 * 全局变量
***********************************************************************/
// extern const struct sysmon_event_info g_event_info[];
// extern const linx_event_table_t g_linx_event_table[LINX_EVENT_TYPE_MAX];

#ifdef __cplusplus
}
#endif

#endif /* __LINX_EVENTS_PUBLIC_H__  */