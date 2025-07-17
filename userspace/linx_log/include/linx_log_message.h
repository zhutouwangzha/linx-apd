#ifndef __LINX_LOG_MESSAGE_H__
#define __LINX_LOG_MESSAGE_H__ 

#include <sys/time.h>

#include "linx_log_level.h"

typedef struct {
    char *message;
    linx_log_level_t level;
    struct timeval tv;
} linx_log_message_t;

#endif /* __LINX_LOG_MESSAGE_H__ */
