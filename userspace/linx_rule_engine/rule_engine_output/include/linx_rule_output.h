#ifndef __LINX_RULE_OUTPUT_H__
#define __LINX_RULE_OUTPUT_H__

#include "linx_rule_engine_load.h"

/**
 * @brief 格式化并输出规则匹配结果
 * 
 * 将规则的output字段中的变量（如%evt.time, %user.name等）替换为实际值并输出
 * 
 * @param rule 匹配的规则
 * @return int 0表示成功，-1表示失败
 */
int linx_rule_output_format_and_print(const linx_rule_t *rule);

/**
 * @brief 格式化输出字符串
 * 
 * 将output字符串中的变量替换为实际值
 * 
 * @param output_template 输出模板字符串
 * @param formatted_output 格式化后的输出字符串（需要调用者释放）
 * @return int 0表示成功，-1表示失败
 */
int linx_rule_output_format_string(const char *output_template, char **formatted_output);

#endif /* __LINX_RULE_OUTPUT_H__ */