#ifndef __RULE_MATCH_FUNC_H__
#define __RULE_MATCH_FUNC_H__ 

#include <stdbool.h>

bool or_matcher(void *context);

bool and_matcher(void *context);

bool num_gt_matcher(void *context);

bool num_ge_matcher(void *context);

bool num_lt_matcher(void *context);

bool num_le_matcher(void *context);

#endif /* __RULE_MATCH_FUNC_H__ */
