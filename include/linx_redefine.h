#ifndef __LINX_REDEFINE_H__
#define __LINX_REDEFINE_H__

/**
 * 文件相关
 */
#define LINX_O_RDONLY       00000000
#define LINX_O_WRONLY       00000001
#define LINX_O_RDWR         00000002
#define LINX_O_CREAT        00000100
#define LINX_O_EXCL         00000200
#define LINX_O_TRUNC        00001000
#define LINX_O_APPEND       00002000
#define LINX_O_NONBLOCK     00004000
#define LINX_O_DSYNC        00010000
#define LINX_O_DIRECT       00040000
#define LINX_O_LARGEFILE    00100000
#define LINX_O_DIRECTORY    00200000
#define LINX_O_CLOEXEC      02000000

#define LINX_O_SYNC         (04000000 | LINX_O_DSYNC)
#define LINX_O_TMPFILE      (020000000 | LINX_O_DIRECTORY)

#endif /* __LINX_REDEFINE_H__ */
