#ifndef __LINX_THREAD_STATE_H__
#define __LINX_THREAD_STATE_H__

typedef enum {
    LINX_THREAD_STATE_RUNNING,      /* Thread is running */
    LINX_THREAD_STATE_PAUSED,       /* Thread is paused */
    LINX_THREAD_STATE_TERMINATING,  /* Thread is terminating */
    LINX_THREAD_STATE_TERMINATED,   /* Thread is terminated */
    LINX_THREAD_STATE_MAX           /* Maximum number of thread states */
} linx_thread_state_t;

#endif /* __LINX_THREAD_STATE_H__ */