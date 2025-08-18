#ifndef __LINX_REDEFINE_H__
#define __LINX_REDEFINE_H__

/**
 * 文件相关
 */
#define LINX_O_LARGEFILE    0
#define LINX_O_DIRECTORY    0200000
#define LINX_O_DIRECT       040000
#define LINX_O_TRUNC        01000
#define LINX_O_SYNC         04010000
#define LINX_O_NONBLOCK     04000
#define LINX_O_EXCL         0200
#define LINX_O_DSYNC        010000
#define LINX_O_APPEND       02000
#define LINX_O_CREAT        0100
#define LINX_O_RDWR         02
#define LINX_O_WRONLY       01
#define LINX_O_RDONLY       00
#define LINX_O_CLOEXEC      02000000
#define LINX_O_TMPFILE      (020000000 | LINX_O_DIRECTORY)

#endif /* __LINX_REDEFINE_H__ */
