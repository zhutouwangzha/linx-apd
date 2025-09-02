#ifndef __CSTRING_H__
#define __CSTRING_H__

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct cstring_s {
    char *data;
    size_t length;
    size_t capacity;

    /**
     * 容量相关函数
     */
    size_t (*get_length)(const struct cstring_s *cs);
    size_t (*get_capacity)(const struct cstring_s *cs);
    bool (*empty)(const struct cstring_s *cs);
    void (*reserve)(struct cstring_s *cs, size_t new_capacity);
    void (*shrink_to_fit)(struct cstring_s *cs);

    /**
     * 访问元素函数
     */
    char (*at)(const struct cstring_s *cs, size_t index);
    char *(*get_data)(const struct cstring_s *cs);
    const char *(*cstr)(const struct cstring_s* cs);

    /**
     * 修改字符串函数
     */
    void (*clear)(struct cstring_s *cs);
    void (*append)(struct cstring_s *cs, const char *str);
    void (*append_char)(struct cstring_s *cs, char c);
    void (*append_format)(struct cstring_s *cs, const char *format, ...);
    void (*assign)(struct cstring_s *cs, const char *str);
    void (*insert)(struct cstring_s *cs, size_t pos, const char *str);
    void (*erase)(struct cstring_s *cs, size_t pos, size_t len);
    void (*replace)(struct cstring_s *cs, size_t pos, size_t len, const char *str);

    /**
     * 查找函数
     */
    size_t (*find)(const struct cstring_s *cs, const char *str, size_t pos);
    size_t (*rfind)(const struct cstring_s *cs, const char *str, size_t pos);

    /**
     * 比较函数
     */
    int (*compare)(const struct cstring_s *cs, const char *str);
    bool (*equals)(const struct cstring_s *cs, const char *str);

    /**
     * 子字符串
     */
    struct cstring_s *(*substr)(const struct cstring_s *cs, size_t pos, size_t len);
} cstring_t;

/**
 * 创建销毁函数
 */
cstring_t *cstring_create();
cstring_t *cstring_create_from_str(const char *str);
char *cstring_copy_data(cstring_t *cs, bool clean);
void cstring_destroy(cstring_t *cs);

#endif /* __CSTRING_H__ */
