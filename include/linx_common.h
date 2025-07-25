#ifndef __LINX_COMMON_H__
#define __LINX_COMMON_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LINX_MEM_CALLOC(type, var, num, size)   \
    var = (type)calloc(num, size)

#define LINX_MEM_FREE(ptr)                      \
    do {                                        \
        if (ptr) {                              \
            free(ptr);                          \
            ptr = NULL;                         \
        }                                       \
    } while (0)                                 \

#define LINX_ARRAY_SIZE(arr)                    \
    (sizeof(arr) / sizeof(arr[0]))

#define LINX_SNPRINTF(byte_write, dest, size, format, ...)                  \
    ({                                                                      \
        int result;                                                         \
        do {                                                                \
            result = snprintf((dest), (size), (format), ##__VA_ARGS__);     \
            if (result < 0 || result > (size) ||                            \
                (byte_write) + result > (size) - 1)                         \
            {                                                               \
                result = -1;                                                \
                break;                                                      \
            }                                                               \
                                                                            \
            (byte_write) += result;                                         \
        } while (0);                                                        \
        result;                                                             \
    })

#endif /* __LINX_COMMON_H__ */