#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "linx_signal.h"
#include "linx_resource_cleanup.h"

static void linx_signal_handler(int signum)
{
    switch (signum) {
    /* 目前 ctrl + c 和 USR1 都触发清理资源的操作 */
    case SIGINT:
    case SIGUSR1:
        linx_resource_cleanup();
        break;
    default:
        break;
    } 
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
