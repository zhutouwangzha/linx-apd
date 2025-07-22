#ifndef __FIELD_TYPE_H__
#define __FIELD_TYPE_H__ 

/**
 * 支持的数据类型
*/
typedef enum {
    FIELD_TYPE_UNKNOWN = 0,
    FIELD_TYPE_INT8,
    FIELD_TYPE_UINT8,
    FIELD_TYPE_INT16,
    FIELD_TYPE_UINT16,
    FIELD_TYPE_INT32,
    FIELD_TYPE_UINT32,
    FIELD_TYPE_INT64,
    FIELD_TYPE_UINT64,
    FIELD_TYPE_CHARBUF,             /* 可打印的字符串，数组 */
    FIELD_TYPE_BYTEBUF,             /* 二进制数据，不方便打印 */
    FILED_TYPE_CHARBUF_ARRAY,       /* 可打印的字符串指针 */
    FIELD_TYPE_CHARBUF_PAIR_ARRAY,  /* 字符串数组指针*/
    FILED_TYPE_BYTEBUF_ARRAY,       /* 二进制数据指针 */
    FIELD_TYPE_BOOL,
    FIELD_TYPE_FLOAT,
    FIELD_TYPE_DOUBLE,
    FIELD_TYPE_MAX
} field_type_t;

#endif /* __FIELD_TYPE_H__ */
