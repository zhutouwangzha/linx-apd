#ifndef __LINX_RULE_ENGINE_MATCH_H__
#define __LINX_RULE_ENGINE_MATCH_H__ 

#include <stdbool.h>

#include "rule_match_struct.h"
#include "output_match_func.h"

#include "linx_rule_engine_ast.h"

bool linx_rule_engine_match(linx_rule_match_t *match);

int linx_compile_ast(ast_node_t *ast, linx_rule_match_t **match);

#endif /* __LINX_RULE_ENGINE_MATCH_H__ */
