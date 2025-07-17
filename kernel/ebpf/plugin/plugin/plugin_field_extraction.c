#include "linx_common.h"
#include "linx_syscall_table.h"

#include "plugin_log.h"
#include "plugin_field_idx.h"
#include "macro/plugin_fields_macro.h"

#define PLUGIN_FIELDS_MACRO(id, up_name, low_name, type, isList, isRequired, isIndex, isKey, addOutput, display, desc) \
    "\t{\n\t\t\"type\": \""#type"\",\n\t\t\"name\": \"linx."#low_name"\",\n\t\t\"isList\": "#isList",\n\t\t\"arg\": {\n\t\t\t\"isRequired\": "#isRequired",\n\t\t\t\"isIndex\": "#isIndex",\n\t\t\t\"isKey\": "#isKey"\n\t\t},\n\t\t\"addOutput\": "#addOutput",\n\t\t\"display\": "#display",\n\t\t\"desc\": "#desc"\n\t},\n"

static const char *plugin_fields = "[\n"PLUGIN_FIELDS_MACRO_ALL"{\"type\": \"string\", \"name\": \"end\", \"desc\": \"用于通过json检查\"}]\n";

#undef PLUGIN_FIELDS_MACRO

uint16_t* plugin_get_extract_event_types(uint32_t* numtypes, ss_plugin_t* s)
{
    static uint16_t types[] = {
        PPME_PLUGINEVENT_E
    };

    *numtypes = sizeof(types) / sizeof(types[0]);

    return &types[0];
}

const char* plugin_get_extract_event_sources()
{
    return R"(["linx_ebpf"])";
}

const char* plugin_get_fields()
{
    return plugin_fields;
}

ss_plugin_rc plugin_extract_fields(ss_plugin_t* s, const ss_plugin_event_input* evt,
                                    const ss_plugin_field_extract_input* in)
{
    plugin_init_state_t *init_state = (plugin_init_state_t *)s;
    scap_sized_buffer buffer[PLUGIN_NPARAMS];
    linx_event_t *linx_event;
    uint32_t nparams;

    nparams = scap_event_decode_params((const scap_evt *)evt->evt, buffer);
    if (nparams != PLUGIN_NPARAMS ||
        *((uint32_t *)buffer[0].buf) != PLUGIN_ID) {
        return SS_PLUGIN_FAILURE;
    }

    linx_event = (linx_event_t *)buffer[1].buf;

    if (in->fields->field_id >= 0 && in->fields->field_id < PLUGIN_FIELDS_COMM) {
        if (linx_event_rich_msg(&init_state->rich_value, 
                                linx_event, in->fields))
        {
            return SS_PLUGIN_FAILURE;
        }

        in->fields->res.str = (const char **)&init_state->rich_value.value[in->fields->field_id];
        in->fields->res_len = 1;
    } else {
        switch (in->fields->field_id) {
        case PLUGIN_FIELDS_COMM: /* linx.comm */
            /**
             * 返回的是char **,而bpf传出的是 char []，所以需要一个变量中转一下
             * 后续想使用一个指针绑定到comm字符串数组，这样每次就直接返回那个指针即可
             */
            init_state->comm_store = &linx_event->comm[0];
            in->fields->res.str = (const char **)&init_state->comm_store;
            in->fields->res_len = 1;
            break;
        case PLUGIN_FIELDS_PID: /* linx.pid */
            in->fields->res.u64 = &linx_event->pid;
            in->fields->res_len = 1;
            break;
        case PLUGIN_FIELDS_TID: /* linx.tid */
            in->fields->res.u64 = &linx_event->tid;
            in->fields->res_len = 1;
            break;
        case PLUGIN_FIELDS_PPID: /* linx.ppid */
            in->fields->res.u64 = &linx_event->ppid;
            in->fields->res_len = 1;
            break;
        case PLUGIN_FIELDS_CMDLINE: /* linx.cmdline */
            init_state->cmdline_store = &linx_event->cmdline[0];
            in->fields->res.str = (const char **)&init_state->cmdline_store;
            in->fields->res_len = 1;
            break;
        case PLUGIN_FIELDS_FULLPATH: /* linx.fullpath */
            init_state->fullpath_store = &linx_event->fullpath[0];
            in->fields->res.str = (const char **)&init_state->fullpath_store;
            in->fields->res_len = 1;
            break;
        default:
            return SS_PLUGIN_FAILURE;
            break;
        }
    }

    return SS_PLUGIN_SUCCESS;
}
