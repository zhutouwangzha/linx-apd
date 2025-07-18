#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include "linx_yaml.h"
#include "linx_log.h"
#include "linx_rule_engine_load.h"
#include "linx_rule_engine_set.h"
#include "linx_rule_engine_ast.h"

static void test(linx_output_match_t *output_match)
{
    segment_t *segment;

    for (size_t i = 0; i < output_match->size; i++) {
        segment = output_match->segments[i];

        switch (segment->type) {
        case SEGMENT_TYPE_LITERAL:
            printf("%s", segment->data.literal.text);
            break;
        case SEGMENT_TYPE_VARIABLE:
            printf("%% %s.%s", segment->data.variable.table_name, segment->data.variable.field_name);
            break;
        default:
            break;
        }
    }
}

static int linx_rule_engine_add_rule_to_set(linx_yaml_node_t *root)
{
    int ret = 0;
    linx_rule_t *rule;
    ast_node_t *ast_root = NULL;
    linx_rule_match_t *match = NULL;
    linx_output_match_t *output_match = NULL;
    char path_buf[256] = {0};

    if (root == NULL) {
        return -1;
    }

    for (int i = 0; i < root->child_count; ++i) {
        rule = malloc(sizeof(linx_rule_t));

        snprintf(path_buf, sizeof(path_buf), "%d.rule", i);
        rule->name = strdup(linx_yaml_get_string(root, path_buf, "unknown"));

        snprintf(path_buf, sizeof(path_buf), "%d.desc", i);
        rule->desc = strdup(linx_yaml_get_string(root, path_buf, "NO"));

        snprintf(path_buf, sizeof(path_buf), "%d.condition", i);
        rule->condition = strdup(linx_yaml_get_string(root, path_buf, "NO"));

        snprintf(path_buf, sizeof(path_buf), "%d.output", i);
        rule->output = strdup(linx_yaml_get_string(root, path_buf, "NO"));

        snprintf(path_buf, sizeof(path_buf), "%d.priority", i);
        rule->priority = strdup(linx_yaml_get_string(root, path_buf, "ERROR"));

        snprintf(path_buf, sizeof(path_buf), "%d.chdesc", i);
        rule->chdesc = strdup(linx_yaml_get_string(root, path_buf, "NO"));

        snprintf(path_buf, sizeof(path_buf), "%d.notify.title", i);
        rule->notify.title = strdup(linx_yaml_get_string(root, path_buf, "NO"));

        snprintf(path_buf, sizeof(path_buf), "%d.notify.content", i);
        rule->notify.content = strdup(linx_yaml_get_string(root, path_buf, "NO"));

        /* 进行规则到AST的转换 */
        ret = condition_to_ast(rule->condition, &ast_root);
        if (ret) {
            LINX_LOG_ERROR("condition_to_ast failed");
        }

        /* 该函数会释放 ast 因为已经没有用了 */
        ret = linx_compile_ast(ast_root, &match);
        if (ret) {
            LINX_LOG_ERROR("rule %s compile error", rule->name);
        }

        ret = linx_output_match_compile(&output_match, rule->output);
        if (ret) {
            LINX_LOG_ERROR("rule %s output compile error", rule->name);
        }

        test(output_match);

        /* 转换成功则添加到列表中 */
        ret = linx_rule_set_add(rule, match, output_match);
        if (ret) {
            LINX_LOG_ERROR("add rule to rule set failed");
        }
    }

    return ret;
}

static int linx_rule_engine_load_file(const char *file_path)
{
    int ret = 0;
    linx_yaml_node_t *root;

    root = linx_yaml_load(file_path);
    if (!root) {
        return -1;
    }

    printf("\n\nparser rule file is %s\n", file_path);

    fflush(stdout);

    ret = linx_rule_engine_add_rule_to_set(root);
    if (ret) {
        LINX_LOG_ERROR("add %s rule to list failed", file_path);
    }

    linx_yaml_node_free(root);

    return ret;
}

static int linx_rule_engine_load_dir(const char *dir_path)
{
    int ret = 0;
    char full_path[1024] = {0};
    DIR *dir;
    struct dirent *entry;

    dir = opendir(dir_path);
    if (dir == NULL) {
        LINX_LOG_ERROR("open dir %s failed", dir_path);
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        ret = linx_rule_engine_load_file(full_path);
        if (ret) {
            break;
        }
    }
    
    closedir(dir);
    return ret;
}

int linx_rule_engine_load(const char *rules_file_path)
{
    /**
     * rules_file_path 可能是一个文件或者一个路径
     * 这里需要把所有的规则解析到 rule_set 中
     * rule_set 是规则的一个集合，包含
     * 规则原始字段，规则的匹配结构体
     * 
     * 在加载规则时检查语法，错误就退出。
     * */
    struct stat path_stat;
    int ret;

    ret = linx_rule_set_init();
    if (ret) {
        return ret;
    }

    if (stat(rules_file_path, &path_stat) == -1) {
        return -1;
    }

    if (S_ISDIR(path_stat.st_mode)) {
        ret = linx_rule_engine_load_dir(rules_file_path);
    } else if (S_ISREG(path_stat.st_mode)) {
        ret = linx_rule_engine_load_file(rules_file_path);
    } else {
        ret = -1;
    }

    return ret;
}
