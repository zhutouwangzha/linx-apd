#ifndef __LINX_LOG_H__
#define __LINX_LOG_H__ 

#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

#include "linx_log_level.h"
#include "linx_log_message.h"

#define LINX_LOG_DEBUG(...)     linx_log(LINX_LOG_DEBUG,   __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_INFO(...)      linx_log(LINX_LOG_INFO,    __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_WARNING(...)   linx_log(LINX_LOG_WARNING, __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_ERROR(...)     linx_log(LINX_LOG_ERROR,   __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_FATAL(...)     linx_log(LINX_LOG_FATAL,   __FILE__, __LINE__, ##__VA_ARGS__)

#define LINX_LOG_DEBUG_V(format, args)     linx_log_v(LINX_LOG_DEBUG,   __FILE__, __LINE__, format, args)
#define LINX_LOG_INFO_V(format, args)      linx_log_v(LINX_LOG_INFO,    __FILE__, __LINE__, format, args)
#define LINX_LOG_WARNING_V(format, args)   linx_log_v(LINX_LOG_WARNING, __FILE__, __LINE__, format, args)
#define LINX_LOG_ERROR_V(format, args)     linx_log_v(LINX_LOG_ERROR,   __FILE__, __LINE__, format, args)
#define LINX_LOG_FATAL_V(format, args)     linx_log_v(LINX_LOG_FATAL,   __FILE__, __LINE__, format, args)

typedef struct {
    linx_log_level_t level;
    FILE *log_file;
    linx_log_message_t **queue;
    pthread_mutex_t lock;
    int queue_size;
    int queue_capacity;
} linx_log_t;

int linx_log_init(const char *log_file, const char *log_level);

void linx_log_deinit(void);

void linx_log(linx_log_level_t level, const char *file, int line, const char *format, ...);

void linx_log_v(linx_log_level_t level, const char *file, int line, const char *format, va_list args);

#endif /* __LINX_LOG_H__  */