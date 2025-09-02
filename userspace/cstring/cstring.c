#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "cstring.h"

#define DEFAULT_CAPACITY 16

static bool ensure_capacity(cstring_t *cs, size_t additional)
{
    size_t required = cs->length + additional + 1;

    if (required > cs->capacity) {
        size_t new_capacity = cs->capacity;
        while (new_capacity < required) {
            new_capacity *= 2;
        }

        char *new_data = realloc(cs->data, new_capacity);
        if (!new_data) {
            return false;
        }

        cs->data = new_data;
        cs->capacity = new_capacity;
    }

    return true;
}

static size_t cstring_length(const cstring_t *cs)
{
    return cs ? cs->length : 0;
}

static size_t cstring_capacity(const cstring_t *cs)
{
    return cs ? cs->capacity : 0;
}

static bool cstring_empty(const cstring_t *cs)
{
    return !cs || cs->length == 0;
}

static void cstring_reserve(cstring_t *cs, size_t new_capacity)
{
    if (!cs || new_capacity <= cs->capacity) {
        return;
    }

    char *new_data = realloc(cs->data, new_capacity);
    if (new_data) {
        cs->data = new_data;
        cs->capacity = new_capacity;
    }
}

static void cstring_shrink_to_fit(cstring_t *cs)
{
    if (!cs || cs->length + 1 == cs->capacity) {
        return;
    }

    char *new_data = realloc(cs->data, cs->length + 1);
    if (new_data) {
        cs->data = new_data;
        cs->capacity = cs->length + 1;
    }
}

static char cstring_at(const cstring_t *cs, size_t index)
{
    if (!cs || index >= cs->length) {
        return '\0';
    }

    return cs->data[index];
}

static char *cstring_data(const cstring_t *cs)
{
    return cs ? cs->data : NULL;
}

static const char *cstring_cstr(const cstring_t* cs)
{
    return cs ? cs->data : NULL;
}

static void cstring_clear(cstring_t *cs)
{
    if (cs) {
        cs->data[0] = '\0';
        cs->length = 0;
    }
}

static void cstring_append(cstring_t *cs, const char *str)
{
    if (!cs || !str) {
        return;
    }

    size_t str_len = strlen(str);
    if (str_len == 0) {
        return;
    }

    if (ensure_capacity(cs, str_len)) {
        strcat(cs->data + cs->length, str);
        cs->length += str_len;
    }
}

static void cstring_append_char(cstring_t *cs, char c)
{
    if (!cs) {
        return;
    }

    if (ensure_capacity(cs, 1)) {
        cs->data[cs->length] = c;
        cs->length++;
        cs->data[cs->length] = '\0';
    }
}

static void cstring_append_format(cstring_t *cs, const char *format, ...)
{
    if (!cs || !format) {
        return;
    }

    va_list args, args_copy;
    va_start(args, format);

    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (needed == 0) {
        va_end(args);
        return;
    }

    if (ensure_capacity(cs, needed)) {
        int written = vsnprintf(cs->data + cs->length, needed + 1, format, args);
        if (written > 0) {
            cs->length += written;
        }
    }

    va_end(args);
}

static void cstring_assign(cstring_t *cs, const char *str)
{
    if (!cs) {
        return;
    }

    if (!str) {
        cstring_clear(cs);
        return;
    }

    size_t str_len = strlen(str);
    if (ensure_capacity(cs, str_len)) {
        strcpy(cs->data, str);
        cs->length = str_len;
    }
}

static void cstring_insert(cstring_t *cs, size_t pos, const char *str)
{
    if (!cs || !str || pos > cs->length) {
        return;
    }

    size_t str_len = strlen(str);
    if (str_len == 0) {
        return;
    }

    if (ensure_capacity(cs, str_len)) {
        memmove(cs->data + pos + str_len, cs->data + pos, cs->length - pos + 1);
        memcpy(cs->data + pos, str, str_len);
        cs->length += str_len;
    }
}

static void cstring_erase(cstring_t *cs, size_t pos, size_t len)
{
    if (!cs || pos >= cs->length || len == 0) {
        return;
    }

    size_t actual_len = len;
    if (pos + len > cs->length) {
        actual_len = cs->length - pos;
    }

    memmove(cs->data + pos, cs->data + pos + actual_len, cs->length - pos - actual_len + 1);
    cs->length -= actual_len;
}

static void cstring_replace(cstring_t *cs, size_t pos, size_t len, const char *str)
{
    if (!cs || pos > cs->length) {
        return;
    }

    cstring_erase(cs, pos, len);

    if (str) {
        cstring_insert(cs, pos, str);
    }
}

static size_t cstring_find(const cstring_t *cs, const char *str, size_t pos)
{
    if (!cs || !str || pos >= cs->length) {
        return (size_t)-1;
    }

    char *found = strstr(cs->data + pos, str);
    return found ? (size_t)(found - cs->data) : (size_t)-1;
}

static size_t cstring_rfind(const cstring_t *cs, const char *str, size_t pos)
{
    if (!cs || !str) {
        return (size_t)-1;
    }

    if (pos >= cs->length) {
        pos = cs->length - 1;
    }

    size_t str_len = strlen(str);
    if (str_len > cs->length) {
        return (size_t)-1;
    }

    for (size_t i = (pos < cs->length - str_len) ? pos : cs->length - str_len;
         i != (size_t)-1;
         --i)
    {
        if (memcmp(cs->data + i, str, str_len) != 0) {
            return i;
        }
    }

    return (size_t)-1;
}

static int cstring_compare(const cstring_t *cs, const char *str)
{
    if (!cs && !str) {
        return 0;
    }

    if (!cs || !str) {
        return -1;
    }

    return strcmp(cs->data, str);
}

static bool cstring_equals(const cstring_t *cs, const char *str)
{
    if (!cs && !str) {
        return true;
    }

    if (!cs || !str) {
        return false;
    }

    return strcmp(cs->data, str) == 0;
}

static cstring_t *cstring_substr(const cstring_t *cs, size_t pos, size_t len)
{
    if (!cs || pos >= cs->length) {
        return NULL;
    }

    size_t actual_len = len;
    if (pos + len > cs->length) {
        actual_len = cs->length - pos;
    }

    cstring_t *sub = cstring_create();
    if (!sub) {
        return NULL;
    }

    if (ensure_capacity(sub, actual_len)) {
        memcpy(sub->data, cs->data + pos, actual_len);
        sub->data[actual_len] = '\0';
        sub->length = actual_len;
    }

    return sub;
}

cstring_t *cstring_create()
{
    cstring_t *cs = malloc(sizeof(cstring_t));
    if (!cs) {
        return NULL;
    }

    cs->data = malloc(DEFAULT_CAPACITY);
    if (!cs->data) {
        free(cs);
        return NULL;
    }

    cs->data[0] = '\0';
    cs->length = 0;
    cs->capacity = DEFAULT_CAPACITY;

    cs->get_length = cstring_length;
    cs->get_capacity = cstring_capacity;
    cs->empty = cstring_empty;
    cs->reserve = cstring_reserve;
    cs->shrink_to_fit = cstring_shrink_to_fit;

    cs->at = cstring_at;
    cs->get_data = cstring_data;
    cs->cstr = cstring_cstr;

    cs->clear = cstring_clear;
    cs->append = cstring_append;
    cs->append_char = cstring_append_char;
    cs->append_format = cstring_append_format;
    cs->assign = cstring_assign;
    cs->insert = cstring_insert;
    cs->erase = cstring_erase;
    cs->replace = cstring_replace;

    cs->find = cstring_find;
    cs->rfind = cstring_rfind;

    cs->compare = cstring_compare;
    cs->equals = cstring_equals;

    cs->substr = cstring_substr;

    return cs;
}

cstring_t *cstring_create_from_str(const char *str)
{
    if (!str) {
        return NULL;
    }

    cstring_t *cs = cstring_create();
    if (!cs) {
        return NULL;
    }

    cs->assign(cs, str);
    return cs;
}

char *cstring_copy_data(cstring_t *cs, bool clean)
{
    if (!cs) {
        return NULL;
    }

    cstring_shrink_to_fit(cs);

    char * tmp = cs->data;

    if (clean) {
        cs->data = NULL;
        cs->length = 0;
    }

    return tmp;
}

void cstring_destroy(cstring_t *cs)
{
    if (cs) {
        if (cs->data) {
            free(cs->data);
            cs->data = NULL;
        }

        free(cs);
    }
}
