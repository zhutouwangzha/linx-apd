#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cjson/cJSON.h>

#include "plugin_config.h"
#include "linx_common.h"
#include "linx_syscall_table.h"

#include "macro/plugin_open_config_macro.h"

#define PLUGIN_OPEN_CONFIG_MACRO(num, up_name, low_name, ...)    \
    #low_name,

static const char *open_config_members[OPEN_IDX_MAX] = {
    PLUGIN_OPEN_CONFIG_MACRO_ALL
};

#undef PLUGIN_OPEN_CONFIG_MACRO

static void plugin_config_fill_interest_syscall_table(char *file_path)
{
    cJSON *json_config, *member, *interest;
    char *jsonstr;
    struct stat file_stat;
    size_t byte_read;
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        LINX_LOG_ERROR("The '%s' file does not exist!", file_path);
        return;
    }

    if (fstat(fd, &file_stat) == -1) {
        LINX_LOG_ERROR("Failed to get '%s' file stat!", file_path);
        return;
    }

    LINX_MEM_CALLOC(char *, jsonstr, 1, file_stat.st_size);
    if (!jsonstr) {
        LINX_LOG_ERROR("Failed to allco for space for jsonstr!");
        return;
    }

    byte_read = read(fd, jsonstr, file_stat.st_size);
    if (byte_read != file_stat.st_size) {
        LINX_LOG_ERROR("The file length(%d) is inconsitstent with the read length(%d)!",
                       file_stat.st_size, byte_read);
        close(fd);
        return;
    }

    close(fd);

    json_config = cJSON_Parse(jsonstr);
    if (!json_config) {
        LINX_LOG_WARNING("Failed to parse the string into cJSON!");
        return;
    }

    for (int i = 0; i < LINX_SYSCALL_MAX_IDX; ++i) {
        member = cJSON_GetObjectItem(json_config, g_linx_syscall_table[i].name);
        if (!member) {
            LINX_LOG_WARNING("The `%s` member does not exist in the json config!", 
                             g_linx_syscall_table[i].name);
            continue;
        }

        interest = cJSON_GetObjectItem(member, "interesting");
        if (!interest) {
            LINX_LOG_WARNING("The `interesting` member does not exist in the json config!");
            continue;
        }

        g_plugin_config.open_config.interest_syscall_table[i] = interest->valueint;
    }

    cJSON_Delete(json_config);
}

void plugin_config_parse_open_config(const char *config)
{
    cJSON *json_config, *member, *item;
    size_t array_size;
    int array_index = 0, byte_write = 0;

    if (!config) {
        LINX_LOG_WARNING("The string does not exist!");
        return;
    }

    json_config = cJSON_Parse(config);
    if (!json_config) {
        LINX_LOG_WARNING("Failed to parse the string into cJSON!");
        return;
    }

    for (int i = 0; i < LINX_ARRAY_SIZE(open_config_members); ++i) {
        member = cJSON_GetObjectItem(json_config, open_config_members[i]);
        if (!member) {
            LINX_LOG_WARNING("`%s` does not exist in the json config!", open_config_members[i]);
            continue;
        }

        switch (i) {
        case OPEN_IDX_FILTER_OWN: /* filter_own */
            if (strcmp(member->valuestring, "true") == 0) {
                g_plugin_config.open_config.filter_own = 1;
            }
            break;
        case OPEN_IDX_FILTER_FALCO: /* filter_falco */
            if (strcmp(member->valuestring, "true") == 0) {
                g_plugin_config.open_config.filter_falco = 1;
            }
            break;
        case OPEN_IDX_FILTER_PIDS: /* filter_pids */
            array_size = cJSON_GetArraySize(member);
            array_index = 0;

            if (g_plugin_config.open_config.filter_own) {
                g_plugin_config.open_config.filter_pids[array_index++] = getpid();
            }

            /**
             * 目前发现falco的pid就是插件pid+1，所以这里直接加
             * TODO: 后续调研是否有更合理的办法获取falco的pid
             * 并且falco并不是创建了一个子任务，还有其他任务，
             * 需要调研如何获得这些任务的pid
             */
            if (g_plugin_config.open_config.filter_falco) {
                g_plugin_config.open_config.filter_pids[array_index++] = 
                    getpid() + 1;
            }

            for (int idx = 0;
                 idx < array_size && array_index < LINX_BPF_FILTER_PID_MAX_SIZE;
                 ++idx)
            {
                item = cJSON_GetArrayItem(member, idx);
                if (!item) {
                    LINX_LOG_WARNING("The %d element in the `%s` json array does not exist!", 
                                     idx, open_config_members[i]);
                    continue;
                }

                g_plugin_config.open_config.filter_pids[array_index++] = item->valueint;
            }
        case OPEN_IDX_FILTER_COMMS: /* filter_comms */
            array_size = cJSON_GetArraySize(member);
            for (int idx = 0, array_index = 0;
                 idx < array_size && array_index < LINX_BPF_FILTER_COMM_MAX_SIZE;
                 ++idx)
            {
                item = cJSON_GetArrayItem(member, idx);
                if (!item) {
                    LINX_LOG_WARNING("The %d element in the `%s` json array does not exist!", 
                                     idx, open_config_members[i]);
                    continue;
                }

                if (LINX_SNPRINTF(byte_write, 
                                  (char *)g_plugin_config.open_config.filter_comms[array_index],
                                  LINX_COMM_MAX_SIZE, "%s", item->valuestring) > 0)
                {
                    array_index += 1;
                } else {
                    LINX_LOG_WARNING("Failed when copy %d bytes '%s' str to %lu bytes buf for %d time!", 
                                     strlen(item->valuestring), item->valuestring, 
                                     LINX_COMM_MAX_SIZE, array_index);
                }
            }
            break;
        case OPEN_IDX_DROP_MODE:    /* drop_mode */
            if (strcmp(member->valuestring, "true") == 0) {
                g_plugin_config.open_config.drop_mode = 1;
            }
            break;
        case OPEN_IDX_DROP_FAILED:  /* drop_failed */
            if (strcmp(member->valuestring, "true") == 0) {
                g_plugin_config.open_config.drop_failed = 1;
            }
            break;
        case OPEN_IDX_INTEREST_SYSCALL_FILE:    /* interest_syscall_table */
            plugin_config_fill_interest_syscall_table(member->valuestring);
            break;
        default:
            break;
        }
    }

    cJSON_Delete(json_config);
}
