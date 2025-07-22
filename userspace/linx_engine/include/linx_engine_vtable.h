#ifndef __LINX_ENGINE_STRUCT_H__
#define __LINX_ENGINE_STRUCT_H__ 

#include <stdint.h>

#include "linx_event.h"

/**
 * 该结构体是所有引擎需要提供的
 * 这样可以使用回调函数更方便的调用不同引擎
*/
typedef struct {
    const char *name;

    int (*init)(void);
    int (*close)(void);
    int (*next)(linx_event_t **event);
    int (*start)(void);
    int (*stop)(void);
} linx_engine_vtable_t;

#endif /* __LINX_ENGINE_STRUCT_H__ */