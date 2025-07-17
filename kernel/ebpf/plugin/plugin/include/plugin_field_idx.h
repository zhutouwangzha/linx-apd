#ifndef __PLUGIN_FIELD_IDX_H__
#define __PLUGIN_FIELD_IDX_H__

#include "macro/plugin_fields_macro.h"

/**
 * 设计 PLUGIN_FIELDS_COMM 是一个特殊值
 * 该值前面的字段都会走linx_event_rich进一步丰富内容
 * 从该字段开始以及后面的字段都不会走linx_event_rich
 */

#define PLUGIN_FIELDS_MACRO(idx, up_name, ...)  \
    PLUGIN_FIELDS_##up_name = idx,

typedef enum {
    PLUGIN_FIELDS_MACRO_ALL
    PLUGIN_FIELDS_MAX
} plugin_field_idx_t;

#undef PLUGIN_FIELDS_MACRO

#endif /* __PLUGIN_FIELD_IDX_H__ */
