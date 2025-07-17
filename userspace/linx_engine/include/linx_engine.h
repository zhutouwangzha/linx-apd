#ifndef __LINX_ENGINE_H__
#define __LINX_ENGINE_H__ 

#include "linx_config.h"
#include "linx_engine_vtable.h"

typedef struct {
    linx_engine_vtable_t *vtable;
} linx_engine_t;

int linx_engine_init(linx_global_config_t *config);

int linx_engine_close(void);

int linx_engine_next(void);

int linx_engine_start(void);

int linx_engine_stop(void);

#endif /* __LINX_ENGINE_H__  */