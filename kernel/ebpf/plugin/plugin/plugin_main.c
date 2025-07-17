#include "linx_common.h"
#include "linx_event_putfile.h"
#include "plugin_log.h"
#include "plugin_config.h"

#include "macro/plugin_init_config_macro.h"

plugin_init_state_t *g_init_state;

#define PLUGIN_INIT_CONFIG_MACRO(idx, up_name, low_name, value, ...) \
    "    \""#low_name"\": "#value",\n"

static const char *plugin_init_schema = "{\n"PLUGIN_INIT_CONFIG_MACRO_ALL"\"end\":\"用于通过falco-json检查\"\n}";

#undef PLUGIN_INIT_CONFIG_MACRO

const char* plugin_get_init_schema(ss_plugin_schema_type* schema_type)
{
    *schema_type = SS_PLUGIN_SCHEMA_JSON;

    return plugin_init_schema;
}

ss_plugin_t *plugin_init(const ss_plugin_init_input* in, ss_plugin_rc* rc)
{
    plugin_init_state_t *init_state;

    in->log_fn(in->owner, NULL, "Start initializing the plugin...", SS_PLUGIN_LOG_SEV_INFO);

    LINX_MEM_CALLOC(plugin_init_state_t *, init_state, 1, sizeof(plugin_init_state_t));
    if (!init_state) {
        in->log_fn(in->owner, NULL, "Failed to allco for space for init_state!", SS_PLUGIN_LOG_SEV_ERROR);
        *rc = SS_PLUGIN_FAILURE;
        return NULL;
    }

    LINX_MEM_CALLOC(char *, init_state->lasterr, 1, PLUGIN_LASTERR_MAXSIZE);
    if (!init_state->lasterr) {
        in->log_fn(in->owner, NULL, "Failed to allco for space for init_state->lasterr!", SS_PLUGIN_LOG_SEV_ERROR);
        *rc = SS_PLUGIN_FAILURE;
        return NULL;
    }

    plugin_config_parse_init_config(in->config);

    linx_log_init(g_plugin_config.init_config.log_level, 
                  g_plugin_config.init_config.log_file);

    if (linx_event_rich_init(&init_state->rich_value)) {
        in->log_fn(in->owner, NULL, "Init linx_event_rich failed!", SS_PLUGIN_LOG_SEV_ERROR);
        *rc = SS_PLUGIN_FAILURE;
        return NULL;
    }

    init_state->log = in->log_fn;
    init_state->owner = in->owner;

    g_init_state = init_state;

    LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_INFO, "The plugin initialization is complete!");

    *rc = SS_PLUGIN_SUCCESS;
    return (ss_plugin_t *)init_state;
}

void plugin_destroy(ss_plugin_t* s)
{
    LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_INFO, "Destroy the plugin ...");

    linx_log_close();
    linx_event_rich_deinit(&((plugin_init_state_t *)s)->rich_value);
    linx_event_putfile_clean();

    LINX_MEM_FREE(((plugin_init_state_t *)s)->lasterr);
    LINX_MEM_FREE(s);

    LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_INFO, "The plugin destroy is complete!");
}

const char *plugin_get_last_error(ss_plugin_t* s)
{
    return ((plugin_init_state_t *)s)->lasterr;
}
