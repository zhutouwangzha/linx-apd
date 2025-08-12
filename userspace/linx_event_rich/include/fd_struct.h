#ifndef __FD_STRUCT_H__
#define __FD_STRUCT_H__ 

#include <stdint.h>

typedef struct {
    int64_t num;            /* 文件描述符唯一数字 */
    char *type;             /* 文件类型 */
    char *typechar;         /* 文件类型，一个字符 */
    char name[128];          /* 完整路径或连接元组 */
    char directory[64];     /* 路径 */
    char filename[64];      /* 文件名 */

    uint32_t ip;
    uint32_t cip;
    uint32_t sip;
    uint32_t lip;
    uint32_t rip;

    uint8_t port;
    uint8_t cport;
    uint8_t sport;
    uint8_t lport;
    uint8_t rport;

    char l4port[4];
} linx_fd_t;

#endif /* __FD_STRUCT_H__ */
