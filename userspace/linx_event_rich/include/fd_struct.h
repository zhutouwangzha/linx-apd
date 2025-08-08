#ifndef __FD_STRUCT_H__
#define __FD_STRUCT_H__ 

#include <stdint.h>

typedef struct {
    int64_t num;
    char *type;
    char *typechar;
    char name[64];
    char directory[64];
    char filename[64];

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
