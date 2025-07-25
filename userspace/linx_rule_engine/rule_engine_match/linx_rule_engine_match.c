#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "linx_log.h"
#include "rule_match_func.h"
#include "linx_rule_engine_ast.h"
#include "linx_rule_engine_match.h"

static linx_rule_match_t *compile_ast(ast_node_t *ast);

static int find_field_name_and_value(ast_node_t *root, char **name, void **ctx)
{
    ast_node_t *left, *right;
    num_context_t **num_ctx;
    str_context_t **str_ctx;

    if (root == NULL || 
        root->data.binary.left == NULL || 
        root->data.binary.right == NULL)
    {
        *name = NULL;
        return -1;
    }

    if (root->data.binary.left->type == AST_NODE_TYPE_FIELD_NAME) {
        left = root->data.binary.left;
        right = root->data.binary.right;
    } else if (root->data.binary.right->type == AST_NODE_TYPE_FIELD_NAME) {
        left = root->data.binary.right;
        right = root->data.binary.left;
    } else {
        *name = NULL;
        return -1;
    }

    *name = left->data.field_name;

    switch (right->type) {
    case AST_NODE_TYPE_INT:
        num_ctx = (num_context_t **)ctx;
        (*num_ctx)->number.int_val = right->data.number_value.int_value;
        break;
    case AST_NODE_TYPE_FLOAT:
        num_ctx = (num_context_t **)ctx;
        (*num_ctx)->number.double_val = right->data.number_value.double_value;
        break;
    case AST_NODE_TYPE_STRING:
        str_ctx = (str_context_t **)ctx;
        (*str_ctx)->str = strdup(right->data.string_value);
        (*str_ctx)->str_len = strlen(right->data.string_value);
        break;
    default:
        break;
    }

    return 0;
}

static linx_rule_match_t *compile_binary_bool_node(ast_node_t *node)
{
    linx_rule_match_t *left = compile_ast(node->data.binary.left);
    linx_rule_match_t *right = compile_ast(node->data.binary.right);
    linx_rule_match_t *match;
    logic_context_t *context;

    if (left == NULL || right == NULL) {
        return NULL;
    }

    match = malloc(sizeof(linx_rule_match_t));
    if (match == NULL) {
        return NULL;
    }

    context = malloc(sizeof(logic_context_t));
    if (context == NULL) {
        return NULL;
    }

    context->left = left;
    context->right = right;
    match->context = context;

    switch (node->data.binary.op.bool_op) {
    case BINARY_BOOL_OP_OR:
        match->func = or_matcher;
        break;
    case BINARY_BOOL_OP_AND:
        match->func = and_matcher;
        break;
    default:
        break;
    }

    return match;
}

static linx_rule_match_t *compile_binary_num_node(ast_node_t *node)
{
    int ret;
    char *field_name;
    linx_rule_match_t *match;
    num_context_t *context;

    match = malloc(sizeof(linx_rule_match_t));
    if (match == NULL) {
        return NULL;
    }

    context = malloc(sizeof(num_context_t));
    if (context == NULL) {
        return NULL;
    }

    ret = find_field_name_and_value(node, &field_name, (void *)&context);
    if (ret) {
        LINX_LOG_ERROR("Failed to find field name");
    }

    context->field = linx_hash_map_get_field_by_path(field_name);

    switch (node->data.binary.op.num_op) {
    case BINARY_NUM_OP_GT:
        match->func = num_gt_matcher;
        break;
    case BINARY_NUM_OP_GE:
        match->func = num_ge_matcher;
        break;
    case BINARY_NUM_OP_LT:
        match->func = num_lt_matcher;
        break;
    case BINARY_NUM_OP_LE:
        match->func = num_le_matcher;
        break;
    default:
        break;
    }

    match->context = context;

    return match;
}

static linx_rule_match_t *compile_binary_str_node(ast_node_t *node)
{
    int ret;
    char *field_name;
    linx_rule_match_t *match;
    str_context_t *context;

    match = malloc(sizeof(linx_rule_match_t));
    if (match == NULL) {
        return NULL;
    }

    context = malloc(sizeof(num_context_t));
    if (context == NULL) {
        free(match);
        return NULL;
    }

    ret = find_field_name_and_value(node, &field_name, (void *)&context);
    if (ret) {
        LINX_LOG_ERROR("Failed to find field name");
    }

    context->field = linx_hash_map_get_field_by_path(field_name);

    switch (node->data.binary.op.str_op) {
    case BINARY_STR_OP_EQ:
        break;
    case BINARY_STR_OP_ASSIGN:
        match->func = str_assign_matcher;
        break;
    case BINARY_STR_OP_NE:
        match->func = str_ne_matcher;
        break;
    case BINARY_STR_OP_GLOB:
        break;
    case BINARY_STR_OP_IGLOB:
        break;
    case BINARY_STR_OP_CONTAINS:
        match->func = str_contains_matcher;
        break;
    case BINARY_STR_OP_ICONTAINS:
        match->func = str_icontains_matcher;
        break;
    case BINARY_STR_OP_BCONTAINS:
        break;
    case BINARY_STR_OP_STARTSWITH:
        match->func = str_startswith_matcher;
        break;
    case BINARY_STR_OP_BSTARTSWITH:
        break;
    case BINARY_STR_OP_ENDSWITH:
        match->func = str_endswith_matcher;
        break;
    case BINARY_STR_OP_REGEX:
        break;
    default:
        break;
    }

    match->context = context;

    return match;
}

static linx_rule_match_t *compile_binary_list_node(ast_node_t *node)
{
    int ret;
    const char *field_name;
    linx_rule_match_t *match;
    num_context_t *context;

    match = malloc(sizeof(linx_rule_match_t));
    if (match == NULL) {
        return NULL;
    }

    context = malloc(sizeof(num_context_t));
    if (context == NULL) {
        return NULL;
    }

    // ret = find_field_name_and_value(node, field_name, context);
    // if (ret) {
    //     LINX_LOG_ERROR("Failed to find field name");
    // }

    switch (node->data.binary.op.list_op) {
    case BINARY_LIST_OP_INTERSECTS:
        break;
    case BINARY_LIST_OP_IN:
        break;
    case BINARY_LIST_OP_PMATCH:
        break;
    default:
        break;
    }

    return match;
}

static linx_rule_match_t *compile_unary_node(ast_node_t *node)
{
    int ret;
    const char *field_name;
    linx_rule_match_t *match;
    num_context_t *context;

    match = malloc(sizeof(linx_rule_match_t));
    if (match == NULL) {
        return NULL;
    }

    context = malloc(sizeof(num_context_t));
    if (context == NULL) {
        return NULL;
    }

    // ret = find_field_name_and_value(node, field_name, context);
    // if (ret) {
    //     LINX_LOG_ERROR("Failed to find field name");
    // }

    switch (node->data.unary.unary_op) {
    case UNARY_OP_EXISTS:
        break;
    case UNARY_OP_NOT:
        break;
    default:
        break;
    }

    return match;
}

static linx_rule_match_t *compile_ast(ast_node_t *ast)
{
    linx_rule_match_t *match = NULL;

    if (ast == NULL) {
        return match;
    }

    switch (ast->type) {
    case AST_NODE_TYPE_BIN_BOOL_OP:
        match = compile_binary_bool_node(ast);
        break;
    case AST_NODE_TYPE_BIN_NUM_OP:
        match = compile_binary_num_node(ast);
        break;
    case AST_NODE_TYPE_BIN_STR_OP:
        match = compile_binary_str_node(ast);
        break;
    case AST_NODE_TYPE_BIN_LIST_OP:
        match = compile_binary_list_node(ast);
        break;
    case AST_NODE_TYPE_UN_OP:
        match = compile_unary_node(ast);
        break;
    default:
        match = NULL;
        break;
    }

    return match;
}

int linx_compile_ast(ast_node_t *ast, linx_rule_match_t **match)
{
    int ret = 0;

    if (ast == NULL) {
        return -1;
    }
    
    *match = compile_ast(ast);

    ast_node_destroy(ast);

    return ret;
}

void linx_rule_engine_match_destroy(linx_rule_match_t *match)
{
    str_context_t *s_context;
    num_context_t *n_context;
    logic_context_t *l_context;

    if (!match) {
        return;
    }

    switch (match->type) {
    case MATCH_CONTEXT_STR:
        s_context = (str_context_t *)match->context;
        if (s_context) {
            if (s_context->str) {
                free(s_context->str);
                s_context->str = NULL;
            }

            free(s_context);
            match->context = NULL;
        }
        break;
    case MATCH_CONTEXT_NUM:
        n_context = (num_context_t *)match->context;
        if (n_context) {
            free(n_context);
            match->context = NULL;
        }
        break;
    case MATCH_CONTEXT_LOGIC:
        l_context = (logic_context_t *)match->context;
        if (l_context) {
            if (l_context->left) {
                linx_rule_engine_match_destroy(l_context->left);
            }

            if (l_context->right) {
                linx_rule_engine_match_destroy(l_context->left);
            }

            free(l_context);
            match->context = NULL;
        }
        break;
    default:
        break;
    }

    free(match);
    match = NULL;
}

bool linx_rule_engine_match(linx_rule_match_t *match)
{
    return match->func(match->context);
}
