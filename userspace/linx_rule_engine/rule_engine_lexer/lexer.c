#include <stdlib.h>
#include <string.h>

#include "linx_log.h"
#include "linx_regex.h"
#include "operation_type.h"
#include "linx_rule_engine_lexer.h"

#define RGX_NOTBLANK "(not[[:space:]]+)"	// 该正则表达式匹配以 “not” 开头，后面跟着一个或多个空白字符的字符串。
#define RGX_IDENTIFIER "([a-zA-Z]+[a-zA-Z0-9_]*)" // 用于匹配 变量名、函数名或标识符 等常见编程语言中的命名规则。
#define RGX_FIELDNAME "([a-zA-Z]+[a-zA-Z0-9_]*(\\.[a-zA-Z]+[a-zA-Z0-9_]*)+)" // 匹配 带点号（.）分隔的标识符
#define RGX_FIELDARGBARESTR "([^][\"'[:space:]]+)" // 匹配不包含方括号、引号或空白字符的连续字符串
#define RGX_HEXNUM "(0[xX][0-9a-fA-F]+)" // 匹配 十六进制数 的标准表示形式
#define RGX_NUMBER "([+\\-]?[0-9]+[\\.]?[0-9]*([eE][+\\-][0-9]+)?)" // 匹配 浮点数和科学计数法表示的数字，包括正负号、小数部分和指数部分。
#define RGX_BARESTR "([^()\"'[:space:]=,]+)" // 匹配不包含括号、引号或空白字符的连续字符串

static inline void linx_lexer_update_last_token(linx_lexer_t *lexer, const char *str)
{
    if (lexer->last_token) {
        free(lexer->last_token);
    }

    lexer->last_token = strdup(str);
}

linx_lexer_t *linx_lexer_create(const char *input)
{
    linx_lexer_t *lexer = (linx_lexer_t *)malloc(sizeof(linx_lexer_t));
    if (lexer) {
        lexer->input = input;
        lexer->idx = 0;
        lexer->line = 1;
        lexer->col = 1;
        lexer->len = strlen(input);
        lexer->last_token = NULL;
    }

    return lexer;
}

void linx_lexer_destroy(linx_lexer_t *lexer)
{
    if (lexer) {
        free(lexer);
    }
}

static inline const char *current_char(linx_lexer_t *lexer)
{
    return lexer->input + lexer->idx;
}

static inline void update_pos_char(const char c, linx_lexer_t *lexer)
{
    lexer->col++;

    if (c == '\r' || c == '\n') {
        lexer->line++;
        lexer->col = 1;
    }

    lexer->idx++;
}

static inline void update_pos_str(const char *str, int len, linx_lexer_t *lexer)
{
    for (int i = 0; i < len; i++) {
        update_pos_char(str[i], lexer);
    }
}

bool linx_lexer_blank(linx_lexer_t *lexer)
{
    bool found = false;

    while (*current_char(lexer) == ' ' || *current_char(lexer) == '\t' || 
           *current_char(lexer) == '\n' || *current_char(lexer) == '\r' ||
           *current_char(lexer) == '\b')
    {
        found = true;
        update_pos_char(*current_char(lexer), lexer);
    }

    return found;
}

bool linx_lexer_find_str(linx_lexer_t *lexer, const char *str)
{
    int len = strlen(str);

    if (strncmp(current_char(lexer), str, len) == 0) {
        linx_lexer_update_last_token(lexer, str);
        update_pos_str(str, len, lexer);
        return true;
    }

    return false;
}

bool linx_lexer_field_name(linx_lexer_t *lexer)
{
    int ret = linx_regex_match(RGX_FIELDNAME, current_char(lexer), &lexer->last_token);
    if (ret <= 0) {
        return false;
    }

    update_pos_str(lexer->last_token, ret, lexer);

    return true;
}

bool linx_lexer_not_blank(linx_lexer_t *lexer)
{
    int ret = linx_regex_match(RGX_NOTBLANK, current_char(lexer), &lexer->last_token);
    if (ret <= 0) {
        return false;
    }

    update_pos_str(lexer->last_token, ret, lexer);

    return true;
}

bool linx_lexer_hex_num(linx_lexer_t *lexer)
{
    int ret = linx_regex_match(RGX_HEXNUM, current_char(lexer), &lexer->last_token);
    if (ret <= 0) {
        return false;
    }

    update_pos_str(lexer->last_token, ret, lexer);

    return true;
}

bool linx_lexer_num(linx_lexer_t *lexer)
{
    int ret = linx_regex_match(RGX_NUMBER, current_char(lexer), &lexer->last_token);
    if (ret <= 0) {
        return false;
    }

    update_pos_str(lexer->last_token, ret, lexer);

    return true;
}

bool linx_lexer_identifier(linx_lexer_t *lexer)
{
    int ret = linx_regex_match(RGX_IDENTIFIER, current_char(lexer), &lexer->last_token);
    if (ret <= 0) {
        return false;
    }

    update_pos_str(lexer->last_token, ret, lexer);

    return true;
}

bool linx_lexer_field_arg_bare_str(linx_lexer_t *lexer)
{
    int ret = linx_regex_match(RGX_FIELDARGBARESTR, current_char(lexer), &lexer->last_token);
    if (ret <= 0) {
        return false;
    }

    update_pos_str(lexer->last_token, ret, lexer);

    return true;
}

static bool linx_lexer_find_operator_list(linx_lexer_t *lexer, const char *list[], int len)
{
    for (int i = 0; i < len; i++) {
        if (linx_lexer_find_str(lexer, list[i])) {
            return true;
        }
    }

    return false;
}

bool linx_lexer_unary_op(linx_lexer_t *lexer)
{
    /* 这里不处理 not 字符串，所以长度减 1 */
    return linx_lexer_find_operator_list(lexer, g_unary_ops, UNARY_OP_MAX - 1);
}

bool linx_lexer_num_op(linx_lexer_t *lexer)
{
    return linx_lexer_find_operator_list(lexer, g_binary_num_ops, BINARY_NUM_OP_MAX);
}

bool linx_lexer_str_op(linx_lexer_t *lexer)
{
    return linx_lexer_find_operator_list(lexer, g_binary_str_ops, BINARY_STR_OP_MAX);
}

bool linx_lexer_list_op(linx_lexer_t *lexer)
{
    return linx_lexer_find_operator_list(lexer, g_binary_list_ops, BINARY_LIST_OP_MAX);
}

bool linx_lexer_field_transformer_type(linx_lexer_t *lexer)
{
    return linx_lexer_find_operator_list(lexer, g_field_transformer_types, FIELD_TRANSFORMER_TYPE_MAX);
}

bool linx_lexer_field_transformer_val(linx_lexer_t *lexer)
{
    return linx_lexer_find_operator_list(lexer, g_field_transformer_vals, FIELD_TRANSFORMER_VAL_MAX);
}

static char *unescape_str(char *str)
{
    int len = strlen(str);
    char *dest = malloc(len);
    char *p;

    if (!dest) {
        free(str);
        return NULL;
    }

    p = dest;

    for (int i = i; i < len; i++) {
        if (str[i] == '\\') {
            i += 1;

            switch (str[i]) {
            case 'b':
                *p++ = '\b';
                break;
            case 'f':
                *p++ = '\f';
                break;
            case 'n':
                *p++ = '\n';
                break;
            case 'r':
                *p++ = '\r';
                break;
            case 't':
                *p++ = '\t';
                break;
            case ' ':
            case '\\':
                *p++ = '\\';
                break;
            case '/':
                *p++ = '/';
                break;
            case '"':
                if (str[0] != str[i]) {
                    LINX_LOG_ERROR("invalid \\\" escape in '-quoted string");
                }

                *p++ = '\"';
                break;
            case '\'':
                if (str[0] != str[i]) {
                    LINX_LOG_ERROR("invalid \\\" escape in '-quoted string");
                }

                *p++ = '\'';
                break;
            case 'x':
            default:
                LINX_LOG_ERROR("unsupported string escape sequence: \\ %c", str[i]);
                break;
            }
        } else {
            *p++ = str[i];
        }
    }

    *p = '\0';
    free(str);

    return dest;
}

bool linx_lexer_quoted_str(linx_lexer_t *lexer)
{
    char prev = '\\';
    char delimiter = *current_char(lexer);
    int len = 1;

    if (*current_char(lexer) == '\'' ||
        *current_char(lexer) == '\"')
    {
        if (lexer->last_token) {
            free(lexer->last_token);
        }

        lexer->last_token = malloc(len);

        while (*current_char(lexer) != '\0') {
            if (*current_char(lexer) == delimiter &&
                prev != '\\')
            {
                update_pos_char(*current_char(lexer), lexer);
                
                lexer->last_token = realloc(lexer->last_token, len + 1);
                lexer->last_token[len - 1] = delimiter;
                lexer->last_token[len] = '\0';

                lexer->last_token = unescape_str(lexer->last_token);

                return true;
            }

            prev = *current_char(lexer);
            lexer->last_token = realloc(lexer->last_token, len + 1);
            lexer->last_token[len - 1] = prev;
            len += 1;

            update_pos_char(*current_char(lexer), lexer);
        }
        
        return true;
    }

    return false;
}

bool linx_lexer_bare_str(linx_lexer_t *lexer)
{
    int ret = linx_regex_match(RGX_BARESTR, current_char(lexer), &lexer->last_token);
    if (ret <= 0) {
        return false;
    }

    update_pos_str(lexer->last_token, ret, lexer);

    return true;
}
