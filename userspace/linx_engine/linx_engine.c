#include <string.h>

#include "linx_engine.h"
#include "linx_engine_ebpf.h"
#include "linx_engine_kmod.h"
#include "linx_log.h"

static linx_engine_t linx_engine;

int linx_engine_init(linx_global_config_t *config)
{
    /**
     * 通过全局配置，选择那个采集模块
     * 并且根据参数初始化采集模块
    */
    if (strcmp(config->engine.kind, "ebpf") == 0) {
        linx_engine.vtable = &ebpf_vtable;
    } else if (strcmp(config->engine.kind, "kmod") == 0) {
        linx_engine.vtable = &kmod_vtable;
    } else {
        LINX_LOG_ERROR("Do not have right engine!");
    }

    return linx_engine.vtable->init();
}

int linx_engine_close(void)
{
    return linx_engine.vtable->close();
}

int linx_engine_next(linx_event_t **event)
{
    return linx_engine.vtable->next(event);
}

int linx_engine_start(void)
{
    return linx_engine.vtable->start();
}

int linx_engine_stop(void)
{
    return linx_engine.vtable->stop();
}

void linx_engine_cleanup(void)
{
    linx_engine_stop();
    linx_engine_close();

    linx_engine.vtable = NULL;
}
