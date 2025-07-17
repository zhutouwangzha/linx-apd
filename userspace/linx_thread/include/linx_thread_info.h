#ifndef __LINX_THREAD_INFO_H__
#define __LINX_THREAD_INFO_H__ 

#include <pthread.h>

#include "linx_thread_state.h"

typedef struct {
    pthread_t thread_id;
    linx_thread_state_t state;
    pthread_cond_t pause_cond;
    pthread_mutex_t state_mutex;
    int index;
} linx_thread_info_t;

#endif /* __LINX_THREAD_INFO_H__  */
