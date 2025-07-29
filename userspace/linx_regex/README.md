# LINX Regex - 正则表达式模块

## 📋 模块概述

`linx_regex` 是系统的正则表达式处理模块，为规则引擎提供正则表达式匹配功能，支持复杂的字符串模式匹配和文本处理。

## 🎯 核心功能

- **正则编译**: 编译和缓存正则表达式
- **模式匹配**: 高效的字符串模式匹配
- **捕获组**: 支持捕获组和命名捕获组
- **性能优化**: 正则表达式缓存和重用

## 🔧 核心接口

```c
// 正则表达式编译和匹配
int linx_regex_compile(const char *pattern, linx_regex_t *regex);
int linx_regex_match(linx_regex_t *regex, const char *text);
int linx_regex_search(linx_regex_t *regex, const char *text, linx_match_t *matches);

// 便捷函数
bool linx_regex_is_match(const char *pattern, const char *text);
int linx_regex_replace(const char *pattern, const char *text, 
                       const char *replacement, char **result);

// 资源管理
void linx_regex_free(linx_regex_t *regex);
```

## 📊 支持的正则特性

- 基本匹配：`.` `*` `+` `?` `[]` `()` `|`
- 字符类：`\d` `\w` `\s` `\D` `\W` `\S`
- 锚点：`^` `$` `\b` `\B`
- 量词：`{n}` `{n,}` `{n,m}`
- 捕获组：`()` `(?:)` `(?P<name>)`

## 🔗 模块依赖

- **pcre2**: PCRE2正则表达式库