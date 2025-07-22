#ifndef __LINX_RULE_ENGINE_LOAD_H__
#define __LINX_RULE_ENGINE_LOAD_H__ 

typedef struct {
    char *name;
    char *desc;
    char *condition;
    char *output;
    char *priority;
    char **tags;
    char *chdesc;
    struct {
        char *title;
        char *content;
    } notify;
} linx_rule_t;

int linx_rule_engine_load(const char *rules_file_path);

linx_rule_t *linx_rule_create(void);

void linx_rule_destroy(linx_rule_t *rule);

#endif /* __LINX_RULE_ENGINE_LOAD_H__ */
