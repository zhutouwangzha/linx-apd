#ifndef __LINX_RULE_ENGINE_LOAD_H__
#define __LINX_RULE_ENGINE_LOAD_H__ 

typedef struct {
    const char *name;
    const char *desc;
    const char *condition;
    const char *output;
    const char *priority;
    const char **tags;
    const char *chdesc;
    struct {
        const char *title;
        const char *content;
    } notify;
} linx_rule_t;

int linx_rule_engine_load(const char *rules_file_path);

#endif /* __LINX_RULE_ENGINE_LOAD_H__ */
