#include "linx_common.h"
#include "linx_syscall_table.h"
#include "linx_event_putfile.h"

#include "plugin_log.h"
#include "plugin_config.h"

#include "macro/plugin_open_config_macro.h"

#define PLUGIN_OPEN_CONFIG_MACRO(idx, up_name, low_name, ...)    \
    "    {\n        \"value\": \""#low_name "\",\n        \"desc\": " #__VA_ARGS__ "\n    },\n"

static const char *plugin_open_params = "[\n"PLUGIN_OPEN_CONFIG_MACRO_ALL"]\n";

#undef PLUGIN_OPEN_CONFIG_MACRO

uint32_t plugin_get_id()
{
	return PLUGIN_ID;
}

const char *plugin_get_event_source()
{
    return "linx_ebpf";
}

const char* plugin_list_open_params(ss_plugin_t* s, ss_plugin_rc* rc)
{
    *rc = SS_PLUGIN_SUCCESS;
    
    return plugin_open_params;
}

ss_instance_t *plugin_open(ss_plugin_t *s, const char *params, ss_plugin_rc *rc)
{
    plugin_open_state_t *open_state;

    LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_INFO, "Enbale plugin capture...");

    LINX_MEM_CALLOC(plugin_open_state_t *, open_state, 1, sizeof(plugin_open_state_t));
    if (!open_state) {
        LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_ERROR, "Failed to allco for space for open_state!");
        *rc = SS_PLUGIN_FAILURE;
        return NULL;
    }

    LINX_MEM_CALLOC(ss_plugin_event *, open_state->evt, 1, PLUGIN_EVENT_MAX_SIZE);
    if (!open_state->evt) {
        LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_ERROR, "Failed to allco for space for open_state->evt!");
        *rc = SS_PLUGIN_FAILURE;
        return NULL;
    }

    plugin_config_parse_open_config(params);

    if (linx_ebpf_init(&open_state->bpf_manager)) {
        LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_ERROR, "Init linx_ebpf failed!");
        *rc = SS_PLUGIN_FAILURE;
        return NULL;
    }

    LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_INFO, "The plugin capture enable is completed!");

    *rc = SS_PLUGIN_SUCCESS;
    return (ss_instance_t *)open_state;
}

void plugin_close(ss_plugin_t* s, ss_instance_t* i)
{
    LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_INFO, "Close plugin capture...");

    LINX_MEM_FREE(((plugin_open_state_t *)i)->evt);
    LINX_MEM_FREE(i);

    LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_INFO, "The plugin capture close is completed!");
}

ss_plugin_rc plugin_next_batch(ss_plugin_t* s, ss_instance_t* h, uint32_t* nevts, ss_plugin_event*** evts)
{
    plugin_init_state_t *init_state = (plugin_init_state_t *)s;
    plugin_open_state_t *open_state = (plugin_open_state_t *)h;
    scap_sized_buffer buffer1, buffer2;
    linx_event_t *event;
    char error[SCAP_LASTERR_SIZE];
    int32_t scap_ret;

    *nevts = 0;

    scap_ret = linx_ebpf_get_ringbuf_msg(&open_state->bpf_manager);
    if (scap_ret < 0) {
        LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_WARNING, "Failed to get data from linx_ebpf ringbuf!");
        return SS_PLUGIN_FAILURE;
    } else if (scap_ret == 0) {
        return SS_PLUGIN_TIMEOUT;
    }

    buffer1.buf = open_state->evt;
    buffer1.size = PLUGIN_EVENT_MAX_SIZE;

    buffer2.buf = open_state->bpf_manager.ringbuf_data;
    buffer2.size = LINX_EVENT_MAX_SIZE;

    scap_ret = scap_event_encode_params(buffer1, NULL, error,
                                        PPME_PLUGINEVENT_E, PLUGIN_NPARAMS,
                                        PLUGIN_ID, buffer2);
    if (scap_ret != SCAP_SUCCESS) {
        LINX_PLUGIN_LOG(SS_PLUGIN_LOG_SEV_ERROR, "The call to the SCAP encode interface failed!");
        strcpy(init_state->lasterr, error);
        return SS_PLUGIN_FAILURE;
    }

    event = (linx_event_t *)open_state->bpf_manager.ringbuf_data;
    open_state->evt->ts = event->time;

    *nevts = 1;
    *evts = &open_state->evt;

    linx_event_putfile(open_state->bpf_manager.ringbuf_data);

    return SS_PLUGIN_SUCCESS;
}
