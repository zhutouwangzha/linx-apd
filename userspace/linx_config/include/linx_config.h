#ifndef __LINX_CONFIG_H__
#define __LINX_CONFIG_H__ 

typedef struct {
    const char *engine;
    struct {
        const char *output;
        const char *log_level;
    } log_config;
} linx_global_config_t;

int linx_config_init(void);

void linx_config_deinit(void);

linx_global_config_t *linx_config_get(void);

int linx_config_load(const char *config_file);

int linx_config_reload(void);

#endif /* __LINX_CONFIG_H__ */