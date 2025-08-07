#ifndef __LINX_RULE_ENGINE_LEXER_H__
#define __LINX_RULE_ENGINE_LEXER_H__ 

#include <stdbool.h>

typedef struct {
    const char *input;      /* 规则字符串 */
    unsigned int line;      /* 行数 */
    unsigned int col;       /* 列数 */
    unsigned int idx;       /* 当前字符索引 */
    unsigned int len;       /* 当前字符串长度 */
    char *last_token;       /* 上一个词法单元 */
} linx_lexer_t;

linx_lexer_t *linx_lexer_create(const char *input);

void linx_lexer_destroy(linx_lexer_t *lexer);

bool linx_lexer_blank(linx_lexer_t *lexer);

bool linx_lexer_find_str(linx_lexer_t *lexer, const char *str);

bool linx_lexer_field_name(linx_lexer_t *lexer);

bool linx_lexer_not_blank(linx_lexer_t *lexer);

bool linx_lexer_hex_num(linx_lexer_t *lexer);

bool linx_lexer_num(linx_lexer_t *lexer);

bool linx_lexer_identifier(linx_lexer_t *lexer);

bool linx_lexer_field_arg_bare_str(linx_lexer_t *lexer);

bool linx_lexer_unary_op(linx_lexer_t *lexer);

bool linx_lexer_num_op(linx_lexer_t *lexer);

bool linx_lexer_str_op(linx_lexer_t *lexer);

bool linx_lexer_list_op(linx_lexer_t *lexer);

bool linx_lexer_field_transformer_type(linx_lexer_t *lexer);

bool linx_lexer_field_transformer_val(linx_lexer_t *lexer);

bool linx_lexer_quoted_str(linx_lexer_t *lexer);

bool linx_lexer_bare_str(linx_lexer_t *lexer);

#endif /* __LINX_RULE_ENGINE_LEXER_H__ */
