#ifndef __LINX_LOG_H__
#define __LINX_LOG_H__

#include <stdio.h>

typedef enum {
    LINX_LOG_DEBUG,     /* 调试信息 */
    LINX_LOG_INFO,      /* 常规信息 */
    LINX_LOG_WARNING,   /* 警告信息 */
    LINX_LOG_ERROR,     /* 错误信息 */
    LINX_LOG_FATAL,     /* 严重错误信息 */
    LINX_LOG_MAX
} linx_log_level_t;

typedef struct {
    linx_log_level_t log_level;
    FILE            *log_file;
} linx_log_t;

void linx_log_init(linx_log_level_t level, const char *log_file);

void linx_log_close(void);

void linx_log_message(linx_log_level_t level, const char *file, int line, 
                      const char *format, ...);

void linx_log_message_v(linx_log_level_t level, const char *file, int line, 
                        const char *format, va_list args);

#endif /* __LINX_LOG_H__ */
