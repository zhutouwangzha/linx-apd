#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>

#include "linx_signal.h"
#include "linx_resource_cleanup.h"

static volatile sig_atomic_t in_cleanup = 0;

static void linx_signal_handler(int signum)
{
    sigset_t block_set, old_set;

    if (in_cleanup) {
        return;
    }

    sigemptyset(&block_set);
    sigaddset(&block_set, signum);
    if (sigprocmask(SIG_BLOCK, &block_set, &old_set) == -1) {
        perror("sigprocmask failed");
        _exit(EXIT_FAILURE);
    }

    switch (signum) {
    /* 目前 ctrl + c 和 USR1 都触发清理资源的操作 */
    case SIGINT:
    case SIGUSR1:
        in_cleanup = 1;
        linx_resource_cleanup();
        break;
    default:
        break;
    }

    if (sigprocmask(SIG_SETMASK, &old_set, NULL) == -1) {
        perror("sigprocmask failed");
        _exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void linx_setup_signal(int signum)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = linx_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(signum, &sa, NULL) == -1) {
        perror("sigaction 注册失败");
        exit(EXIT_FAILURE);
    }
}
