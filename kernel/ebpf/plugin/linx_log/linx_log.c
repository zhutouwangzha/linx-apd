#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "linx_log.h"

static linx_log_t g_linx_log = {0};

const char *linx_log_level_str[LINX_LOG_MAX] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "FATAL"
};

void linx_log_init(linx_log_level_t level, const char *log_file)
{
    FILE *fp;

    if (log_file) {
        fp = fopen(log_file, "a");
        if (fp) {
            g_linx_log.log_file = fp;
        } else {
            g_linx_log.log_file = stderr;
        }
    } else {
        g_linx_log.log_file = stderr;
    }

    g_linx_log.log_level = level;
}

void linx_log_close(void)
{
    if (g_linx_log.log_file && g_linx_log.log_file != stderr) {
        fclose(g_linx_log.log_file);
    }

    g_linx_log.log_file = NULL;
}

void linx_log_message_v(linx_log_level_t level, const char *file, int line, 
                        const char *format, va_list args)
{
    struct timeval tv;
    struct tm *tm;
    char time_buf[64];

    if (level < g_linx_log.log_level) {
        return;
    }

    gettimeofday(&tv, NULL);

    tm = localtime(&tv.tv_sec);

    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm);

    fprintf(g_linx_log.log_file, "%s.%03ld [%s] [%s:%d] ",
            time_buf, tv.tv_usec / 1000,
            linx_log_level_str[level],
            file, line);
    
    vfprintf(g_linx_log.log_file, format, args);

    fprintf(g_linx_log.log_file, "\n");
    fflush(g_linx_log.log_file);

    if (level == LINX_LOG_FATAL) {
        exit(EXIT_FAILURE);
    }
}

void linx_log_message(linx_log_level_t level, const char *file, int line, 
                      const char *format, ...)
{
    if (level < g_linx_log.log_level) {
        return;
    }

    va_list args;
    va_start(args, format);
    linx_log_message_v(level, file, line, format, args);
    va_end(args);
}
