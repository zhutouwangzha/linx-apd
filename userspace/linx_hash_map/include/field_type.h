#ifndef __FIELD_TYPE_H__
#define __FIELD_TYPE_H__ 

/**
 * 支持的数据类型
*/
typedef enum {
    FIELD_TYPE_UNKNOWN = 0,
    FIELD_TYPE_INT32,
    FIELD_TYPE_INT64,
    FIELD_TYPE_STRING,
    FIELD_TYPE_BOOL,
    FIELD_TYPE_FLOAT,
    FIELD_TYPE_DOUBLE,
    FIELD_TYPE_MAX
} field_type_t;

#endif /* __FIELD_TYPE_H__ */
