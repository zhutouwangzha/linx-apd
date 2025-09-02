#ifndef __LINX_FD_INFO_H__
#define __LINX_FD_INFO_H__ 

#include <stdint.h>

#include "linx_fd_type.h"
#include "linx_proto_type.h"

#include "uthash_ext.h"

typedef struct {
    int64_t num;            /* 文件描述符唯一数字 */
    uint64_t ino;           /* inode */
    union {
        char *str;
        linx_fd_type_t num;
    } type;                 /* 文件类型 */
    char typechar;          /* 文件类型，一个字符 */
    char name[128];         /* 完整路径或连接元组 */
    char directory[64];     /* 路径 */
    char filename[64];      /* 文件名 */

    char *ip;               /* 匹配fd的IP地址（客户端或服务端） */
    char cip[64];           /* 客户端IP */
    char sip[64];           /* 服务端IP */
    char lip[64];           /* 本地IP */
    char rip[64];           /* 远端IP */

    char *net;
    char cnet[70];
    char snet[70];
    char lnet[70];
    char rnet[70];

    uint16_t port;
    uint16_t cport;
    uint16_t sport;
    uint16_t lport;
    uint16_t rport;

    char *l4proto;
    UT_hash_handle hh;
} linx_fd_info_t;

typedef struct {
    char *string;       /* 文件类型字符串 */
    char str;           /* 文件类型单个字符 */
} linx_fd_type_str_t;

typedef struct {
    int64_t net_ns;
    linx_fd_info_t *sockets;
    UT_hash_handle hh;
} linx_fd_socket_list_t;

int linx_fd_info_bind_field(void);

linx_fd_info_t *linx_fd_info_create(pid_t pid, int64_t fd);

void linx_fd_info_destroy(linx_fd_info_t *fd_info);

void linx_fd_info_cleanup(linx_fd_info_t *fdlist);

void linx_fd_socket_list_free(linx_fd_socket_list_t **list);

char *linx_fd_type_string_get(linx_fd_type_t f_type);

char linx_fd_type_str_get(linx_fd_type_t f_type);

char *linx_proto_type_string_get(linx_proto_type_t type);

#endif /* __LINX_FD_INFO_H__ */
