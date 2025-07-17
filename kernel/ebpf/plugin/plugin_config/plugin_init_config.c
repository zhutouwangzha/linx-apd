#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <cjson/cJSON.h>

#include "plugin_config.h"
#include "linx_common.h"
#include "linx_event_putfile.h"

#include "macro/plugin_init_config_macro.h"

#define PLUGIN_INIT_CONFIG_MACRO(num, up_name, low_name, ...)    \
    #low_name,

static const char *init_config_members[INIT_IDX_MAX] = {
    PLUGIN_INIT_CONFIG_MACRO_ALL
};

#undef PLUGIN_INIT_CONFIG_MACRO

static void plugin_config_fill_linx_log_level(const char *log_level_str)
{
    extern const char *linx_log_level_str[LINX_LOG_MAX];
    int flag = 1;

    for (int i = 0; i < LINX_LOG_MAX; ++i) {
        if (strcmp(linx_log_level_str[i], log_level_str) == 0) {
            flag = 0;
            g_plugin_config.init_config.log_level = i;
            break;
        }
    }

    if (flag) {
        LINX_LOG_WARNING("The setting of linx_log_level failed. By default, the ERROR log level is used!");
    }
}

void plugin_config_parse_init_config(const char *config)
{
    cJSON *json_config, *member;
    size_t len;

    if (!config) {
        LINX_LOG_WARNING("The string does not exist!");
        return;
    }

    json_config = cJSON_Parse(config);
    if (!json_config) {
        LINX_LOG_WARNING("Failed to parse the string into cJSON!");
        return;
    }

    for (int i = 0; i < LINX_ARRAY_SIZE(init_config_members); ++i) {
        member = cJSON_GetObjectItem(json_config, init_config_members[i]);
        if (!member) {
            LINX_LOG_WARNING("`%s` does not exist in the json config!", init_config_members[i]);
            continue;
        }

        switch (i) {
        case INIT_IDX_RICH_VALUE_SIZE: /* rich_value_size */
            g_plugin_config.init_config.rich_value_size = member->valueint;
            break;
        case INIT_IDX_STR_MAX_SIZE: /* str_max_size */
            g_plugin_config.init_config.str_max_size = member->valueint;
            break;
        case INIT_IDX_PUTFILE: /* putfile */
            if (strcmp(member->valuestring, "true") == 0) {
                g_plugin_config.init_config.output_config.put_flag = 1;
            }
            break;
        case INIT_IDX_PATH: /* path */
            len = strlen(member->valuestring);
            if (len > LINX_PATH_MAX_SIZE) {
                LINX_LOG_WARNING("The path length(%d) exceeds the acceptable length(%d)!", 
                                 len, LINX_PATH_MAX_SIZE);
                break;
            }
            strcpy(g_plugin_config.init_config.output_config.path, member->valuestring);

            if (g_plugin_config.init_config.output_config.put_flag) {
                if (linx_event_open_file(g_plugin_config.init_config.output_config.path,
                                         &g_plugin_config.init_config.output_config.fd) < 0)
                {
                    LINX_LOG_ERROR("Open event file '%s' Failed!", 
                                   g_plugin_config.init_config.output_config.path);
                }
            }
            break;
        case INIT_IDX_FORMAT: /* format */
            if (strcmp(member->valuestring, "json") == 0) {
                g_plugin_config.init_config.output_config.format = 1;
            }
            break;
        case INIT_IDX_LOG_LEVEL: /* log_level */
            plugin_config_fill_linx_log_level(member->valuestring);
            break;
        case INIT_IDX_LOG_FILE: /* log_file */
            strcpy(g_plugin_config.init_config.log_file, member->valuestring);
            break;
        default:
            break;
        }
    }

    cJSON_Delete(json_config);
}
