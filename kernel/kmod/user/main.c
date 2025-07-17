#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <signal.h>

#include "../include/kmod_msg.h"
#include "../include/user/linx_syscall.h"
#include "linx_logs.h"
#include "linx_devset.h"

#define CDEV_NAME   "/dev/clinx0"

typedef struct kmod_handle
{
    struct kmod_device *dev;
    struct kmod_ops *ops;
}kmod_handle_t;


extern const struct kmod_ops linx_kmod_engine;


static struct kmod_device* kmod_dev;

struct event_header *event_data = NULL;
kmod_device_t g_kmod_dev;


struct option long_opts[] = {
		{"file", 1, 0, 'f'},
        {"test", 0, 0, 't'},
        {"help", 0, 0, 'h'},
		{0, 0, 0, 0}
};


static void usage(const char *name)
{
  printf("Usage: %s <Operation1>  <Operation file>""\n",
         name);
  printf("      Operation1: -f | -t |.\n");
  printf("            -f: Output path of log file.\n");
  printf("            -t: Test the log API.\n");
  printf("            -h: Print internal usage information and exit.\n");
  printf("\t%s -f log.txt\n", name);
  printf("\t%s -t \n", name);
}


int linx_test_txt(void)
{
    char buf[256] = {0};
    int fd = open ("2.txt", O_RDWR);
    if (fd < 0) {
        perror("open\n");
        return -1;
    }

    read(fd, buf, 32);
    printf("buf =%s fd = %d \n", buf, fd);
    close(fd);
    return 0;
}

static void signal_callback(int signal) {
    struct kmod_ops *kmod_engine = (struct kmod_ops *)&linx_kmod_engine;

    print_print_all_event(&g_kmod_dev);
    kmod_engine->komd_close(&g_kmod_dev);
	
	exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    int opt;
    int op_mode;
    int my_argc = argc;
    char **my_argv = argv;
    char *op_file = NULL;
    int ret;
    int i = 0;

    struct kmod_ops *g_kmod_engine = (struct kmod_ops *)&linx_kmod_engine;

    if(signal(SIGINT, signal_callback) == SIG_ERR) {
		fprintf(stderr, "An error occurred while setting SIGINT signal handler.\n");
		return EXIT_FAILURE;
	}

    while ((opt = getopt_long(my_argc, my_argv, "f:th", long_opts, NULL)) != -1) {
        if (ret)
            break;

        switch (opt) {
            case 'f':
                op_file = optarg;
                break;
            case 't':
                linx_logs_test();
                return 0;
            case 'h':
                usage(argv[0]);
                return 0;
            break;
                default:
                ret = -1;
                break;
        }
    }

    if (ret < 0) {
        usage(argv[0]);
        goto out;
    }

    init_syscall_names();
    g_kmod_engine->kmod_init(&g_kmod_dev, op_file);

    unsigned long syscall_nr = __NR_unlinkat;
    g_kmod_engine->kmod_config(&g_kmod_dev, SYSMON_IOCTL_ENABLE_SYSCALL, &syscall_nr);

    while (1)
    {
        if (g_kmod_engine->komd_next(&g_kmod_dev) < 0) {
            sleep(1);
        }
        i++;
    }
    

    // g_kmod_engine->komd_close(&g_kmod_dev);
    signal_callback(-1);

    return 0;

out:

    return ret;
}
