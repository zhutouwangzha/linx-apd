#include "linx_common.h"
#include "linx_event_rich.h"
#include "linx_event_get.h"

#include "plugin_config.h"
#include "plugin_field_idx.h"

static int linx_event_rich_msg_has_args(ss_plugin_extract_field *field, linx_event_t *event, char *buf)
{
    int ret = 0;
    /* 当字段存在参数时 */
    if (field->arg_present) {
        if (!strcmp(field->arg_key, "key")) {
            switch (field->field_id) {
            case PLUGIN_FIELDS_TYPE:
                ret = linx_event_get_syscall_id(event, buf, g_plugin_config.init_config.rich_value_size);
                if (ret < 0) {
                    LINX_LOG_ERROR("Failed to get syscall_id!");
                }
                break;
            case PLUGIN_FIELDS_USER:
                ret = linx_event_get_user_id(event, buf, g_plugin_config.init_config.rich_value_size);
                if (ret < 0) {
                    LINX_LOG_ERROR("Failed to get user_id!");
                }
                break;
            case PLUGIN_FIELDS_GROUP:
                ret = linx_event_get_group_id(event, buf, g_plugin_config.init_config.rich_value_size);
                if (ret < 0) {
                    LINX_LOG_ERROR("Failed to get group_id!");
                }
                break;
            case PLUGIN_FIELDS_FDS:
                ret = linx_event_get_fds_id(event, buf, g_plugin_config.init_config.rich_value_size);
                if (ret < 0) {
                    LINX_LOG_ERROR("Failed to get fds_id!");
                }
                break;
            default:
                LINX_LOG_WARNING("The current enum valeu(%d) does not match any of the known values!", 
                                 field->field_id);
                ret = -1;
                break;
            }
        } else if (!strcmp(field->arg_key, "value")) {
            switch (field->field_id) {
            case PLUGIN_FIELDS_TYPE:
                ret = linx_event_get_syscall_name(event, buf, g_plugin_config.init_config.rich_value_size);
                if (ret < 0) {
                    LINX_LOG_ERROR("Failed to get syscall_name!");
                }
                break;
            case PLUGIN_FIELDS_USER:
                ret = linx_event_get_user(event, buf, g_plugin_config.init_config.rich_value_size);
                if (ret < 0) {
                    LINX_LOG_ERROR("Failed to get user_name!");
                }
                break;
            case PLUGIN_FIELDS_GROUP:
                ret = linx_event_get_group(event, buf, g_plugin_config.init_config.rich_value_size);
                if (ret < 0) {
                    LINX_LOG_ERROR("Failed to get group_name!");
                }
                break;
            case PLUGIN_FIELDS_FDS:
                ret = linx_event_get_fds_name(event, buf, g_plugin_config.init_config.rich_value_size);
                if (ret < 0) {
                    LINX_LOG_ERROR("Failed to get fds_name!");
                }
                break;
            default:
                LINX_LOG_WARNING("The current enum valeu(%d) does not match any of the known values!", 
                                 field->field_id);
                ret = -1;
                break;
            }
        } else {
            ret = -1;
        }
    } else {
        switch (field->field_id) {
        case PLUGIN_FIELDS_TYPE:
            ret = linx_event_get_syscall(event, buf, g_plugin_config.init_config.rich_value_size);
            if (ret < 0) {
                LINX_LOG_ERROR("Failed to get syscall!");
            }
            break;
        case PLUGIN_FIELDS_USER:
            ret = linx_event_get_user_all(event, buf, g_plugin_config.init_config.rich_value_size);
            if (ret < 0) {
                LINX_LOG_ERROR("Failed to get user!");
            }
            break;
        case PLUGIN_FIELDS_GROUP:
            ret = linx_event_get_group_all(event, buf, g_plugin_config.init_config.rich_value_size);
            if (ret < 0) {
                LINX_LOG_ERROR("Failed to get group!");
            }
            break;
        case PLUGIN_FIELDS_FDS:
            ret = linx_event_get_fds(event, buf, g_plugin_config.init_config.rich_value_size);
            if (ret < 0) {
                LINX_LOG_ERROR("Failed to get fds!");
            }
            break;
        default:
            LINX_LOG_WARNING("The current enum valeu(%d) does not match any of the known values!", 
                             field->field_id);
            ret = -1;
            break;
        }
    }

    return ret;
}

int linx_event_rich_msg(linx_event_rich_t *rich, linx_event_t *event, ss_plugin_extract_field *field)
{
    int ret = 0, offset = field->field_id;
    char *tmp = (char *)rich->value[offset];

    switch (offset) {
    case PLUGIN_FIELDS_TYPE: /* linx.type */
    case PLUGIN_FIELDS_USER: /* linx.user */
    case PLUGIN_FIELDS_GROUP: /* linx.group */
    case PLUGIN_FIELDS_FDS: /* linx.fds */
        ret = linx_event_rich_msg_has_args(field, event, tmp);
        if (ret < 0) {
            LINX_LOG_ERROR("Failed to rich msg has args!");
        }
        break;
    case PLUGIN_FIELDS_ARGS: /* linx.args */
        ret = linx_event_get_args(event, tmp, g_plugin_config.init_config.rich_value_size);
        if (ret < 0) {
            LINX_LOG_ERROR("Failed to get args!");
        }
        break;
    case PLUGIN_FIELDS_TIME: /* linx.time */
        ret = linx_event_get_time(event, tmp, g_plugin_config.init_config.rich_value_size);
        if (ret < 0) {
            LINX_LOG_ERROR("Failed to get time!");
        }
        break;
    case PLUGIN_FIELDS_DIR: /* linx.dir */
        ret = linx_event_get_dir(event, tmp, g_plugin_config.init_config.rich_value_size);
        if (ret < 0) {
            LINX_LOG_ERROR("Failed to get dir!");
        }
        break;
    default:
        LINX_LOG_WARNING("The current enum valeu(%d) does not match any of the known values!", 
                         offset);
        ret = -1;
        break;
    }

    if (ret > 0) {
        ret = 0;
    }

    return ret;
}

int linx_event_rich_init(linx_event_rich_t *rich)
{
    for (int i = 0; i < PLUGIN_FIELDS_COMM; ++i) {
        LINX_MEM_CALLOC(void *, rich->value[i], 1, g_plugin_config.init_config.rich_value_size);
        if (!rich->value[i]) {
            LINX_LOG_ERROR("Failed to allco for space for linx event rich value[%d]!", i);
            return -1;
        }
    }

    return 0;
}

void linx_event_rich_deinit(linx_event_rich_t *rich)
{
    for (int i = 0; i < PLUGIN_FIELDS_COMM; ++i) {
        LINX_MEM_FREE(rich->value[i]);
    }
}
