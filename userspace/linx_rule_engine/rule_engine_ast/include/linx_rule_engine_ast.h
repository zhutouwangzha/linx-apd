#ifndef __LINX_RULE_ENGINE_AST_H__
#define __LINX_RULE_ENGINE_AST_H__ 

#include "ast_node.h"

int condition_to_ast(const char *condition, ast_node_t **ast_root);

#endif /* __LINX_RULE_ENGINE_AST_H__ */
