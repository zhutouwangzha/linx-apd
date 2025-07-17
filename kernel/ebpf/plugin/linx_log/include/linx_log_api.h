#ifndef __LINX_LOG_API_H__
#define __LINX_LOG_API_H__

#include "linx_log.h"

#define LINX_LOG_DEBUG(...)                 linx_log_message(LINX_LOG_DEBUG,        __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_INFO(...)                  linx_log_message(LINX_LOG_INFO,         __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_WARNING(...)               linx_log_message(LINX_LOG_WARNING,      __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_ERROR(...)                 linx_log_message(LINX_LOG_ERROR,        __FILE__, __LINE__, ##__VA_ARGS__)
#define LINX_LOG_FATAL(...)                 linx_log_message(LINX_LOG_FATAL,        __FILE__, __LINE__, ##__VA_ARGS__)

#define LINX_LOG_DEBUG_V(format,    args)   linx_log_message_v(LINX_LOG_DEBUG,      __FILE__, __LINE__, format, args)
#define LINX_LOG_INFO_V(format,     args)   linx_log_message_v(LINX_LOG_INFO,       __FILE__, __LINE__, format, args)
#define LINX_LOG_WARNING_V(format,  args)   linx_log_message_v(LINX_LOG_WARNING,    __FILE__, __LINE__, format, args)
#define LINX_LOG_ERROR_V(format,    args)   linx_log_message_v(LINX_LOG_ERROR,      __FILE__, __LINE__, format, args)
#define LINX_LOG_FATAL_V(format,    args)   linx_log_message_v(LINX_LOG_FATAL,      __FILE__, __LINE__, format, args)

#endif /* __LINX_LOG_API_H__ */
