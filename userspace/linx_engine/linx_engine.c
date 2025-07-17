#include "linx_engine.h"

// static linx_engine_t linx_engine;

int linx_engine_init(linx_global_config_t *config)
{
    /**
     * 通过全局配置，选择那个采集模块
     * 并且根据参数初始化采集模块
    */
   (void)config;

    // return linx_engine.vtable->init();
    return 0;
}

int linx_engine_close(void)
{
    // return linx_engine.vtable->close();
    return 0;
}

int linx_engine_next(void)
{
    // return linx_engine.vtable->next();
    return 0;
}

int linx_engine_start(void)
{
    // return linx_engine.vtable->start();
    return 0;
}

int linx_engine_stop(void)
{
    // return linx_engine.vtable->stop();
    return 0;
}
