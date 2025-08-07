#include <stdlib.h>
#include <string.h>

#include "linx_rule_engine_ast.h"
#include "linx_regex.h"
#include "linx_log.h"
#include "linx_rule_engine_lexer.h"

static ast_node_t *parse_embedded_remainder(linx_lexer_t *lexer);
static ast_node_t *parse_field_remainder(linx_lexer_t *lexer);
static ast_node_t *parse_field_or_transformer_remainder(linx_lexer_t *lexer);

static ast_node_t *try_parse_transformer_or_val(linx_lexer_t *lexer)
{
    ast_node_t *node = NULL;
    int op_type;

    linx_lexer_blank(lexer);

    if (linx_lexer_field_transformer_val(lexer)) {
        linx_lexer_blank(lexer);

        op_type = op_str_to_field_transformer_val(lexer->last_token);

        if (!linx_lexer_field_name(lexer)) {
            LINX_LOG_ERROR("Expected field name after transformer");
            return NULL;
        }

        node = parse_field_remainder(lexer);

        linx_lexer_blank(lexer);

        if (!linx_lexer_find_str(lexer, ")")) {
            LINX_LOG_ERROR("Expected ')' after field transformer");
            ast_node_destroy(node);
            return NULL;
        }

        return ast_node_create_field_transformer_val(op_type, node);
    }

    if (linx_lexer_field_transformer_type(lexer)) {
        linx_lexer_blank(lexer);

        return parse_field_or_transformer_remainder(lexer);
    }

    return NULL;
}

static ast_node_t *parse_num_value_or_transformer(linx_lexer_t *lexer)
{
    ast_node_t *node;

   linx_lexer_blank(lexer);

    node = try_parse_transformer_or_val(lexer);
    if (node != NULL) {
        return node;
    }

    if (linx_lexer_hex_num(lexer)) {
        node = ast_node_create_number((long long)strtoul(lexer->last_token, NULL, 16));
        return node;
    } else if (linx_lexer_num(lexer)) {
        if (strstr(lexer->last_token, ".")) {
            node = ast_node_create_float((long double)strtof(lexer->last_token, NULL));
        } else {
            node = ast_node_create_number((long long)strtol(lexer->last_token, NULL, 10));
        }

        return node;
    }

    LINX_LOG_ERROR("expected a number value");

    return NULL;
}

static ast_node_t *parse_str_value_or_transformer(linx_lexer_t *lexer, bool no_transformer)
{
    ast_node_t *node;

    linx_lexer_blank(lexer);

    if (!no_transformer) {
        node = try_parse_transformer_or_val(lexer);
        if (node != NULL) {
            return node;
        }
    }

    /* 解析带引号字符串或裸字符串 */
    if (linx_lexer_quoted_str(lexer) || 
        linx_lexer_bare_str(lexer))
    {
        node = ast_node_create_string(lexer->last_token);
        return node;
    }

    if (no_transformer) {
        LINX_LOG_ERROR("expected a string value");
    }

    LINX_LOG_ERROR("expected a string value or a field with a valid transformer: ");

    return NULL;
}

static ast_node_t *parse_list_value_or_transformer(linx_lexer_t *lexer)
{
    ast_node_t *item, *list;
    bool should_empty = false;

    list = ast_node_create_list();

    linx_lexer_blank(lexer);

    if (linx_lexer_find_str(lexer, "(")) {
        linx_lexer_blank(lexer);

        item = parse_str_value_or_transformer(lexer, true);
        if (item == NULL) {
            should_empty = true;
        }

        if (!should_empty) {
            add_item_to_list(list, item);

            linx_lexer_blank(lexer);

            while (linx_lexer_find_str(lexer, ",")) {
                item = parse_str_value_or_transformer(lexer, true);
                if (item == NULL ||
                    item->data.string_value == NULL) 
                {
                    LINX_LOG_ERROR("parser fatal error: null value expr in body of list");
                }

                add_item_to_list(list, item);

                linx_lexer_blank(lexer);
            }
        }

        if (!linx_lexer_find_str(lexer, ")")) {
            LINX_LOG_ERROR("expected a ')' token");
        }

        return list;
    }

    item = try_parse_transformer_or_val(lexer);
    if (!item) {
        return item;
    }

    if (linx_lexer_identifier(lexer)) {
        // todo
    }

    return NULL;
}

static ast_node_t *parse_condition(linx_lexer_t *lexer, ast_node_t *left)
{
    ast_node_t *right, *node;
    int op_type;

    linx_lexer_blank(lexer);

    /* 处理一元运算符 */
    if (linx_lexer_unary_op(lexer)) {
        op_type = (int)op_str_to_unary_op(lexer->last_token);
        return ast_node_create_unary(op_type, left);
    }

    linx_lexer_blank(lexer);

    /* 处理二元运算符 */
    if (linx_lexer_num_op(lexer)) {
        op_type = (int)op_str_to_binary_num_op(lexer->last_token);
        right = parse_num_value_or_transformer(lexer);
        node = ast_node_create_binary_num(op_type, left, right);
    } else if (linx_lexer_str_op(lexer)) {
        op_type = (int)op_str_to_binary_str_op(lexer->last_token);
        right = parse_str_value_or_transformer(lexer, false);
        node = ast_node_create_binary_str(op_type, left, right);
    } else if (linx_lexer_list_op(lexer)) {
        op_type = (int)op_str_to_binary_list_op(lexer->last_token);
        right = parse_list_value_or_transformer(lexer);
        node = ast_node_create_binary_list(op_type, left, right);
    } else {
        LINX_LOG_ERROR("Invalid operator");
    }

    return node;
}

static ast_node_t *parse_field_remainder(linx_lexer_t *lexer)
{
    ast_node_t *node;
    char *field_name = strdup(lexer->last_token);

    if (linx_lexer_find_str(lexer, "[")) {
        if (!linx_lexer_quoted_str(lexer) &&
            !linx_lexer_field_arg_bare_str(lexer))
        {
            LINX_LOG_ERROR("Invalid field argument");
            return NULL;
        }

        field_name = strcat(field_name, ".");
        field_name = strcat(field_name, lexer->last_token);

        if (!linx_lexer_find_str(lexer, "]")) {
            LINX_LOG_ERROR("Missing ']'");
            return NULL;
        }
    }

    node = ast_node_create_field_name(field_name, NULL);
    free(field_name);

    return node;
}

static ast_node_t *parse_field_or_transformer_remainder(linx_lexer_t *lexer)
{
    ast_node_t *node;
    int op_type = op_str_to_field_transformer_type(lexer->last_token);

    linx_lexer_blank(lexer);

    if (linx_lexer_field_transformer_type(lexer)) { 
        linx_lexer_blank(lexer);
        node = parse_field_or_transformer_remainder(lexer);
    }

    if (linx_lexer_field_name(lexer)) {
        node = parse_field_remainder(lexer);
    }

    if (!node) {
        LINX_LOG_ERROR("Missing field name or transformer");
        return NULL;
    }

    linx_lexer_blank(lexer);

    if (!linx_lexer_find_str(lexer, ")")) {
        LINX_LOG_ERROR("Missing ')'");
        return NULL;
    }

    return ast_node_create_field_transformer(op_type, node);
}

static ast_node_t *parse_check(linx_lexer_t *lexer)
{
    ast_node_t *left = NULL;

    linx_lexer_blank(lexer);

    // 处理括号表达式
    if (linx_lexer_find_str(lexer, "(")) {
        return parse_embedded_remainder(lexer);
    }

    // 处理字段表达式
    if (linx_lexer_field_name(lexer)) {
        left = parse_field_remainder(lexer);
        return parse_condition(lexer, left);
    }

    // 处理字段转换表达式
    if (linx_lexer_field_transformer_type(lexer)) {
        linx_lexer_blank(lexer);
        left = parse_field_or_transformer_remainder(lexer);
        return parse_condition(lexer, left);
    }

    // 处理标识符表达式
    if (linx_lexer_identifier(lexer)) {
        // todo
    }

    LINX_LOG_ERROR("expected a '(' token, a field check, or an identifier");

    return NULL;
}

static ast_node_t *parse_not(linx_lexer_t *lexer)
{
    ast_node_t *left, *node;
    bool is_not = false;

    linx_lexer_blank(lexer);

    while (linx_lexer_not_blank(lexer)) {
        is_not = !is_not;
    }

    if (linx_lexer_find_str(lexer, "not(")) {
        is_not = !is_not;
        left = parse_embedded_remainder(lexer);
    } else {
        left = parse_check(lexer);
    }

    if (is_not) {
        node = ast_node_create_unary(UNARY_OP_NOT, left);
    } else {
        node = left;
    }

    return node;
}

static ast_node_t *parse_and(linx_lexer_t *lexer)
{
    ast_node_t *left, *right, *node = NULL;
    
    linx_lexer_blank(lexer);
    left = parse_not(lexer);
    linx_lexer_blank(lexer);

    while (linx_lexer_find_str(lexer, g_binary_bool_ops[BINARY_BOOL_OP_AND])) {
        if (!linx_lexer_blank(lexer)) {
            if (linx_lexer_find_str(lexer, "(")) {
                right = parse_embedded_remainder(lexer);
            } else {
                LINX_LOG_ERROR("expected blank or '(' after 'and'");
            }
        } else {
            right = parse_not(lexer);
        }

        node = ast_node_create_binary_bool(BINARY_BOOL_OP_AND, left, right);
        left = node;

        linx_lexer_blank(lexer);
    }

    return node ? node : left;
}

static ast_node_t *parse_or(linx_lexer_t *lexer)
{
    ast_node_t *left, *right, *node = NULL;
    
    linx_lexer_blank(lexer);
    left = parse_and(lexer);
    linx_lexer_blank(lexer);

    while (linx_lexer_find_str(lexer, g_binary_bool_ops[BINARY_BOOL_OP_OR])) {
        if (!linx_lexer_blank(lexer)) {
            if (linx_lexer_find_str(lexer, "(")) {
                right = parse_embedded_remainder(lexer);
            } else {
                LINX_LOG_ERROR("expected blank or '(' after 'or'");
            }
        } else {
            right = parse_and(lexer);
        }

        node = ast_node_create_binary_bool(BINARY_BOOL_OP_OR, left, right);
        left = node;

        linx_lexer_blank(lexer);
    }

    return node ? node : left;
}

static ast_node_t *parse_embedded_remainder(linx_lexer_t *lexer)
{
    ast_node_t *node;

    linx_lexer_blank(lexer);
    node = parse_or(lexer);
    linx_lexer_blank(lexer);

    if (!linx_lexer_find_str(lexer, ")")) {
        LINX_LOG_ERROR("expected a ')' token");
    }

    return node;
}

static void print_ast(ast_node_t *node)
{
    if (node == NULL) {
        return;
    }

    switch (node->type) {
    case AST_NODE_TYPE_BIN_BOOL_OP:
        print_ast(node->data.binary.left);
        printf(" %s ", g_binary_bool_ops[node->data.binary.op.bool_op]);
        print_ast(node->data.binary.right);
        break;
    case AST_NODE_TYPE_BIN_LIST_OP:
        print_ast(node->data.binary.left);
        printf(" %s ", g_binary_list_ops[node->data.binary.op.list_op]);
        print_ast(node->data.binary.right);
        break;
    case AST_NODE_TYPE_BIN_STR_OP:
        print_ast(node->data.binary.left);
        printf(" %s ", g_binary_str_ops[node->data.binary.op.str_op]);
        print_ast(node->data.binary.right);
        break;
    case AST_NODE_TYPE_BIN_NUM_OP:
        print_ast(node->data.binary.left);
        printf(" %s ", g_binary_num_ops[node->data.binary.op.num_op]);
        print_ast(node->data.binary.right);
        break;
    case AST_NODE_TYPE_UN_OP:
        printf(" %s ", g_unary_ops[node->data.unary.unary_op]);
        print_ast(node->data.unary.operand);
        break;
    case AST_NODE_TYPE_LIST:
        printf("(");
        for (int i = 0; i < node->data.list.count; i++) {
            print_ast(node->data.list.items[i]);
            if (i == node->data.list.count - 1) {
                printf(")");
            } else {
                printf(", ");
            }
        }
        break;
    case AST_NODE_TYPE_FIELD_TRANSFORMER:
        printf(" %s ", g_field_transformer_types[node->data.field_transformer.type.type]);
        print_ast(node->data.field_transformer.operand);
        printf(")");
        break;
    case AST_NODE_TYPE_FIELD_TRANSFORMER_VAL:
        printf(" %s ", g_field_transformer_vals[node->data.field_transformer.type.val]);
        print_ast(node->data.field_transformer.operand);
        printf(")");
        break;
    case AST_NODE_TYPE_INT:
        printf("%lld", node->data.number_value.int_value);
        break;
    case AST_NODE_TYPE_FLOAT:
        printf("%Lf", node->data.number_value.double_value);
        break;
    case AST_NODE_TYPE_STRING:
        printf("%s", node->data.string_value);
        break;
    case AST_NODE_TYPE_FIELD_NAME:
        printf("%s", node->data.field.name);
        break;
    default:
        break;
    }
}

int condition_to_ast(const char *condition, ast_node_t **ast_root)
{
    int ret = 0;
    linx_lexer_t *lexer = linx_lexer_create(condition);

    /* 判断输入是否为空 */
    if (condition == NULL) {
        return -1;
    }

    /* 创建语法树，将逻辑or表达式作为根节点 */
    *ast_root = parse_or(lexer);
    if (*ast_root == NULL) {
        ret = -1;
    }

    print_ast(*ast_root);
    printf("\n");

    linx_lexer_destroy(lexer);

    return ret;
}
