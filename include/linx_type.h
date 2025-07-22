#ifndef __LINX_TYPE_H__
#define __LINX_TYPE_H__

typedef enum {
    LINX_TYPE_NONE = 0,
    LINX_TYPE_INT8 = 1,
    LINX_TYPE_INT16 = 2,
    LINX_TYPE_INT32 = 3,
    LINX_TYPE_INT64 = 4,
    LINX_TYPE_UINT8 = 5,
    LINX_TYPE_UINT16 = 6,
    LINX_TYPE_UINT32 = 7,
    LINX_TYPE_UINT64 = 8,
    LINX_TYPE_CHARBUF = 9,
    LINX_TYPE_BYTEBUF = 10,
    LINX_TYPE_MAX
} linx_type_t;

typedef enum {
    LINX_SYSCALL_TYPE_ENTER = 0,
    LINX_SYSCALL_TYPE_EXIT = 1,
    LINX_SYSCALL_TYPE_MAX
} linx_syscall_type_t;

#endif /* __LINX_TYPE_H__ */
