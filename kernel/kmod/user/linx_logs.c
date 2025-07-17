#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>
#include <malloc.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "kmod_msg.h"
#include "linx_syscall.h"
#include "linx_logs.h"

#define MAX_PRINT_LEN   2048
#define DEFAULT_LOG_FILE   "./linx_kmod.txt"


static const char *syscall_names[__NR_syscalls];

void init_syscall_names(void)
{
    #define MAP_SYSCALL(name) \
        if (__NR_##name < __NR_syscalls) \
            syscall_names[__NR_##name] = #name;
    MAP_SYSCALL(open);
    MAP_SYSCALL(openat);
    MAP_SYSCALL(read);
    MAP_SYSCALL(write);
    MAP_SYSCALL(close);
    MAP_SYSCALL(execve);
    MAP_SYSCALL(clone);
    MAP_SYSCALL(unlink);
    MAP_SYSCALL(unlinkat);
    MAP_SYSCALL(socket);
    MAP_SYSCALL(connect);
    MAP_SYSCALL(bind);
    MAP_SYSCALL(listen);
    MAP_SYSCALL(sendto);
    MAP_SYSCALL(sendmsg);
    MAP_SYSCALL(recvfrom);
    MAP_SYSCALL(recvmsg);
    #undef MAP_SYSCALL 
}


// 获取系统调用名称
static const char *get_syscall_name(uint32_t id)
{
    if (id >=0 && id < __NR_syscalls && syscall_names[id])
        return syscall_names[id];
    
    return "unknown";
}

// 格式化时间戳
static void linx_format_ts(uint64_t ns, char *buf, size_t buflen)
{
    time_t sec = ns / 1000000000;
    long nsec = ns % 1000000000;
    struct tm tm;

    localtime_r(&sec, &tm);
    strftime(buf, buflen, "%Y-%m-%d %H:%M:%S", &tm);

    char *ptr = buf + strlen(buf);
    snprintf(ptr, buflen - (ptr - buf), ".%09ld", nsec);
}

static void print_ip_address(char *buf, size_t len, uint32_t ip)
{
    struct in_addr addr;
    addr.s_addr = ip;
    strcpy(buf, inet_ntoa(addr));
    // snprintf(buf, len, "%d.%d.%d.%d", (ip >> 0) &0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
}

static const char *linx_protocol2str(uint8_t protocol)
{
    switch (protocol)
    {
    case IPPROTO_TCP: return "TCP";
    case IPPROTO_UDP: return "UDP";
    case IPPROTO_ICMP: return "ICMP";    
    default:    return "OTHER";
    }
}

static void linx_print_net_log(struct komd_net_info *net)
{
    char saddr_str[16], daddr_str[16];

    print_ip_address(saddr_str, sizeof(saddr_str), net->saddr);
    print_ip_address(daddr_str, sizeof(daddr_str), net->daddr);

    printf("addr=%x, port=%x net->protocol=%d\n", net->saddr, net->sport, net->protocol);
    if (net->direction == 0) {  // 出站
        printf("-> %s:%d via %s \n", daddr_str, net->dport, linx_protocol2str(net->protocol));
    } else {
        printf("<- %s:%d via %s\n", saddr_str, net->dport, linx_protocol2str(net->protocol));
    }
}


static void linx_log_format(event_header_t *evt, char *buf, size_t buflen, bool fds_enabel)
{
    char time_buf[128] = {0};
    // uint64_t args[6] = {0};
    char fds_buf[128] = {0};
    char *fds_ptr = fds_buf;
    int nbytes = 0;

    event_header_t tmpevt = {0};
    memcpy(&tmpevt, evt, sizeof(event_header_t));

    linx_format_ts(evt->ts, time_buf, sizeof(time_buf));

    // for (size_t i = 0; i < 6; i++)
    // {
    //     args[i] = *(uint64_t *)(evt->args + i * sizeof(uint64_t));
    // }

    // 打印网络信息
    if (evt->net_flag) {
        linx_print_net_log(&evt->net);
        }

    if (fds_enabel) {
        /*TODO: 解析文件描述符对应文件 */
    }
    
    if (fds_enabel) {
        snprintf(buf, buflen, "[%s]: %s syscall=%u dir=%s res=%d user=%s comm=%s cmdline=%s pid=%d tid=%d ppid=%d(%s) fds=%s\n", 
                    time_buf, get_syscall_name(evt->syscall_nr), evt->syscall_nr, 
                    evt->type ? "<" : ">", evt->res, evt->user ? "user" : "root",
                    evt->proc, evt->cmdline, evt->pid, evt->tid,
                    evt->ppid, evt->pp_proc, fds_buf);
    } else {
        snprintf(buf, buflen, "[%s]: %s syscall=%u dir=%s res=%d user=%s comm=%s cmdline=%s pid=%d tid=%d ppid=%d(%s)\n",
                    time_buf, get_syscall_name(evt->syscall_nr), evt->syscall_nr, 
                    evt->type ? "<" : ">", evt->res,evt->user ? "user" : "root",
                    evt->proc, evt->cmdline, evt->pid, evt->tid,
                    evt->ppid, evt->pp_proc);
    }

    // 打印进程文件名称
    // for (size_t i = 0; i < 12; i++)
    // {
    //     if (evt->fd_val[i+1] == 0) {
    //         printf("fd_name=%s \n", evt->fd_name[0]);
    //         break;
    //     }
    //     printf("fd_name=%s \n", evt->fd_name[i]);
    // }
    
    

}

void print_print_all_event(kmod_device_t *dev_set)
{
    printf("----------------------------\n");
    printf("\tall events: %u \n \tdrop events: %u\n", dev_set->msg_bufinfo->n_evts, dev_set->msg_bufinfo->n_drop_evts);
    printf("----------------------------\n");
    return;
}

void linx_log_print(event_header_t *evt)
{
    char *buf = malloc(sizeof(char) * MAX_PRINT_LEN);
    if (!buf) {
        perror("kmod_print");
        return;
    }
    
    memset(buf, 0, MAX_PRINT_LEN);

    linx_log_format(evt, buf, MAX_PRINT_LEN, false);
    // linx_log_format(evt, buf, MAX_PRINT_LEN, true);

    printf("%s", buf);
    free(buf);
    return;
}

int linx_logs_open(char *filename)
{
    int logs_fd = -1;

    if (!filename) {
        logs_fd = open(DEFAULT_LOG_FILE, O_RDWR | O_CREAT, 0644);
        if (logs_fd < 0){
            perror("logs_open ");
            return -1;
        }
    } else {
        logs_fd = open(filename, O_RDWR);
        if (logs_fd < 0){
            perror("logs_open ");
            return -1;
        }
    }

    return logs_fd;
}

int linx_logs_write(int  fd, event_header_t *evt)
{
    char buf[512];
    int nbytes = 0;

    linx_log_format(evt, buf, sizeof(buf), false);

    nbytes = write(fd, buf, strlen(buf));
    if (nbytes < strlen(buf) || nbytes < 0) {
        perror("kmod_write: ");
        return -1;
    }

    return 0;
}

int linx_logs_close(int fd)
{
    close(fd);
}


int linx_logs_test(void)
{
    event_header_t data;
    char buf[512] = {0};
    int fd;

    memset(&data, 0, sizeof(event_header_t));

    data.syscall_nr = 263;
    data.ts = 1749102993893615929;
    data.type = 1;
    data.pid = 6666;
    data.tid = 6666;
    data.user = 0;
    data.ppid = 1234;
    data.res = 0;
    data.fd_val[0] = 1;
    data.fd_val[1] = 2;
    data.fd_val[2] = 3;

    strncpy(data.proc, "rm", sizeof(data.proc)-1);
    strncpy(data.pp_proc, "bash", sizeof(data.pp_proc)-1);
    strncpy(data.cmdline, "rm -rf 1.txt", sizeof(data.cmdline)-1);
    // strncpy(data.fd_name[0], "/tmp/1.txt", sizeof(data.fd_name[0])-1);
    // strncpy(data.fd_name[1], "/dev/pts/1", sizeof(data.fd_name[1])-1);

    init_syscall_names();

    linx_log_print(&data);

    fd = linx_logs_open(NULL);
    linx_logs_write(fd, &data);
    linx_logs_close(fd);

    return 0;
}