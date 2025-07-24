#include <stdlib.h>
#include <string.h>

#include "linx_arg_parser.h"

linx_arg_config_t *linx_arg_config = NULL;

static const struct argp_option linx_arg_options[] = {
    /* 长选项名，短选项名，参数名，             选项标志，帮助文档描述，                    选项分组 */
    {"config",      'c',    "<file_name>",          0,          "指定配置文件路径",         0},
    {"rules",       'r',    "<file_name|path>",     0,          "指定规则文件路径或目录",   0},
    {}
};

const char *argp_program_version = "Linx APD 0.0.1";
const char *argp_program_bug_address = "<https://www.linx-info.com>";
static char linx_apd_doc[] = "Linx APD -- A simple APD tool";
static char linx_apd_args_doc[] = "[-c linx_apd.yaml] [-r linx_apd_rules.yaml]";

static error_t linx_arg_parse(int key, char *arg, struct argp_state *state)
{
    (void)state;
    // struct arguments *arguments = state->input;

    switch (key) {
    case 'c':
        linx_arg_config->linx_apd_config = strdup(arg);
        break;
    case 'r':
        linx_arg_config->linx_apd_rules = strdup(arg);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
        break;
    }

    return 0;
}

const struct argp linx_argp = {
    .options = linx_arg_options,
    .parser = linx_arg_parse,
    .args_doc = linx_apd_args_doc,
    .doc = linx_apd_doc,
    .children = NULL,
};

int linx_arg_init(void)
{
    if (linx_arg_config) {
        return 0;
    }

    linx_arg_config = (linx_arg_config_t *)malloc(sizeof(linx_arg_config_t));
    if (linx_arg_config == NULL) {
        return -1;
    }

    return 0;
}

void linx_arg_deinit(void)
{
    if (!linx_arg_config) {
        return;
    }

    if (linx_arg_config->linx_apd_config) {
        free(linx_arg_config->linx_apd_config);
        linx_arg_config->linx_apd_config = NULL;
    }

    if (linx_arg_config->linx_apd_rules) {
        free(linx_arg_config->linx_apd_rules);
        linx_arg_config->linx_apd_rules = NULL;
    }

    free(linx_arg_config);
    linx_arg_config = NULL;
}

const struct argp *linx_argp_get_argp(void)
{
    return &linx_argp;
}

linx_arg_config_t *linx_arg_get_config(void)
{
    return linx_arg_config;
}
