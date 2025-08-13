#ifndef __LINX_FD_INFO_H__
#define __LINX_FD_INFO_H__ 

#include <stdint.h>

#include "linx_fd_type.h"

#include "uthash.h"

typedef struct {
    int64_t num;            /* 文件描述符唯一数字 */
    linx_fd_type_t type;    /* 文件类型 */
    char *typechar;         /* 文件类型，一个字符 */
    char name[128];         /* 完整路径或连接元组 */
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
    UT_hash_handle hh;
} linx_fd_info_t;

int linx_fd_info_bind_field(void);

linx_fd_info_t *linx_fd_info_create(pid_t pid, int64_t fd);

void linx_fd_info_destroy(linx_fd_info_t *fd_info);

void linx_fd_info_cleanup(linx_fd_info_t *fdlist);

const char *linx_fd_type_sting_get(linx_fd_type_t f_type);

#endif /* __LINX_FD_INFO_H__ */
