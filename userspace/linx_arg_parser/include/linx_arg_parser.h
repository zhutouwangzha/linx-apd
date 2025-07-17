#ifndef __LINX_ARG_PARSER_H__
#define __LINX_ARG_PARSER_H__ 

#include <argp.h>

/* 存储命令行解析出来的参数 */
typedef struct linx_arg_config {
    char *linx_apd_config;
    char *linx_apd_rules;
} linx_arg_config_t;

int linx_arg_init(void);

void linx_arg_deinit(void);

const struct argp *linx_argp_get_argp(void);

linx_arg_config_t *linx_arg_get_config(void);

#endif /* __LINX_ARG_PARSER_H__ */
