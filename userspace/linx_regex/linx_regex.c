#include <string.h>

#include "pcre2.h"

#include "linx_regex.h"
#include "linx_log.h"

/**
 * @brief 使用PCRE2库进行正则表达式匹配
 * 
 * @param regex 输入的正则表达式字符串
 * @param str 需要匹配的目标字符串
 * @return int 匹配结果:
 *             - 匹配成功返回匹配长度
 *             - 无匹配返回0
 *             - 错误返回-1
 */
int linx_regex_match(const char *regex, const char *str, char **match_str)
{
    int ret;
    size_t len;
    int errornumber;
    PCRE2_SIZE erroroffset;
    PCRE2_SIZE *ovector, start, end;
    PCRE2_UCHAR buffer[256];
    pcre2_code *re;
    pcre2_match_data *match_data;
    PCRE2_SPTR pcre2_pattern = (PCRE2_SPTR)regex;
    PCRE2_SIZE str_length = (PCRE2_SIZE)strlen(str);

    /* 参数有效性检查 */
    if (regex == NULL || str == NULL) {
        LINX_LOG_ERROR("regex or str is null");
        return -1;
    }

    /* 正则表达式编译阶段 */
    re = pcre2_compile(
        pcre2_pattern,          /* 正则表达式 */
        PCRE2_ZERO_TERMINATED,  /* 模式以0为结尾 */
        0,
        &errornumber,           /* 错误代码 */
        &erroroffset,           /* 错误位置 */
        NULL);                  /* 编译上下文 */
    if (re == NULL) {
        pcre2_get_error_message(errornumber, buffer, sizeof(buffer));
        LINX_LOG_ERROR("PCRE2 compilation failed at offset %d: %s", 
                       (int)erroroffset, buffer);
        return -1;
    }

    /* JIT编译优化，提升匹配性能 */
    ret = pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
    if (ret != 0) {
        pcre2_get_error_message(ret, buffer, sizeof(buffer));
        LINX_LOG_WARNING("JIT compilation warning: %s.", buffer);
    } else {
        LINX_LOG_DEBUG("JIT compiled");
    }

    /* 创建匹配数据缓冲区 */
    match_data = pcre2_match_data_create_from_pattern(re, NULL);
    if (!match_data) {
        LINX_LOG_ERROR("PCRE2 match data creation failed");
        pcre2_code_free(re);
        return -1;
    }

    /* 执行正则匹配 */
    ret = pcre2_match(
        re,                 /* 编译后的模式 */
        (PCRE2_SPTR)str,    /* 目标字符串 */
        str_length,         /* 字符串长度 */
        0,                  /* 启始偏移 */
        PCRE2_ANCHORED,     /* 选项：从字符串启始位置开始匹配，隐含^锚点 */
        match_data,         /* 匹配结果容器 */
        NULL);              /* 匹配上下文 */

    /* 处理匹配结果 */
    if (ret < 0) {
        switch (ret) {
            case PCRE2_ERROR_NOMATCH:
                LINX_LOG_DEBUG("No match");
                break;
            default:
                pcre2_get_error_message(ret, buffer, sizeof(buffer));
                LINX_LOG_ERROR("Matching error %s", buffer);
                break;
        }
    } else {
        /* 提取并打印匹配结果 */
        ovector = pcre2_get_ovector_pointer(match_data);

        for (int i = 0; i < 1; i++) {
            start = ovector[2 * i];
            end = ovector[2 * i + 1];
            len = end - start;

            *match_str = realloc(*match_str, len + 1);
            snprintf(*match_str, len + 1, "%.*s", (int)len, str + start);

            LINX_LOG_DEBUG("Match [%d:%d](%d): %s", start, end, len, *match_str);
        }

        ret = len;
    }

    /* 释放资源 */
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);

    return ret;
}
