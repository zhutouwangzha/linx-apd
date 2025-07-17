#ifndef __PLUGIN_LOG_H__
#define __PLUGIN_LOG_H__

#include "plugin_struct.h"

extern plugin_init_state_t *g_init_state;

#define LINX_PLUGIN_LOG(log_level, str) \
    g_init_state->log(g_init_state->owner, NULL, str, log_level)

#endif /* __PLUGIN_LOG_H__ */
