#ifndef __LINX_PROTO_TYPE_H__
#define __LINX_PROTO_TYPE_H__

typedef enum {
    LINX_PROTO_UNKNOWN,
    LINX_PROTO_NA,
    LINX_PROTO_TCP,
    LINX_PROTO_UDP,
    LINX_PROTO_ICMP,
    LINX_PROTO_RAW,
    LINX_PROTO_MAX
} linx_proto_type_t;

#endif /* __LINX_PROTO_TYPE_H__ */
