#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "linx_log.h"
#include "linx_fd_info.h"
#include "linx_hash_map.h"
#include "linx_process_cache_define.h"

static const char *s_linx_fd_type_string[LINX_FD_TYPE_MAX] = {
    [LINX_FD_TYPE_UNKNOWN] = "<NA>",
	[LINX_FD_TYPE_FILE] = "file",
	[LINX_FD_TYPE_DIRECTORY] = "directory",
	[LINX_FD_TYPE_IPV4_SOCK] = "ipv4",
	[LINX_FD_TYPE_IPV6_SOCK] = "ipv6",
	[LINX_FD_TYPE_IPV4_SERVSOCK] = "ipv4",
	[LINX_FD_TYPE_IPV6_SERVSOCK] = "ipv6",
	[LINX_FD_TYPE_FIFO] = "pipe",
	[LINX_FD_TYPE_UNIX_SOCK] = "unix",
	[LINX_FD_TYPE_EVENT] = "event",
	[LINX_FD_TYPE_UNSUPPORTED] = "<NA>",
	[LINX_FD_TYPE_SIGNALFD] = "signalfd",
	[LINX_FD_TYPE_EVENTPOLL] = "eventpoll",
	[LINX_FD_TYPE_INOTIFY] = "inotify",
	[LINX_FD_TYPE_TIMERFD] = "timerfd",
	[LINX_FD_TYPE_NETLINK] = "netlink",
	[LINX_FD_TYPE_FILE_V2] = "file",
	[LINX_FD_TYPE_BPF] = "bpf",
	[LINX_FD_TYPE_USERFAULTFD] = "userfaultfd",
	[LINX_FD_TYPE_IOURING] = "io_uring",
	[LINX_FD_TYPE_MEMFD] = "memfd",
	[LINX_FD_TYPE_PIDFD] = "pidfd"
};

int linx_fd_info_bind_field(void)
{
	BEGIN_FIELD_MAPPINGS(fd)
		FIELD_MAP(linx_fd_info_t, num, LINX_FIELD_TYPE_INT64)
		FIELD_MAP(linx_fd_info_t, type, LINX_FIELD_TYPE_INT64)
		FIELD_MAP(linx_fd_info_t, typechar, LINX_FIELD_TYPE_CHARBUF_ARRAY)
		FIELD_MAP(linx_fd_info_t, name, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, directory, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, filename, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, ip, LINX_FIELD_TYPE_UINT32)
		FIELD_MAP(linx_fd_info_t, cip, LINX_FIELD_TYPE_UINT32)
		FIELD_MAP(linx_fd_info_t, sip, LINX_FIELD_TYPE_UINT32)
		FIELD_MAP(linx_fd_info_t, lip, LINX_FIELD_TYPE_UINT32)
		FIELD_MAP(linx_fd_info_t, rip, LINX_FIELD_TYPE_UINT32)
		FIELD_MAP(linx_fd_info_t, port, LINX_FIELD_TYPE_UINT8)
		FIELD_MAP(linx_fd_info_t, cport, LINX_FIELD_TYPE_UINT8)
		FIELD_MAP(linx_fd_info_t, sport, LINX_FIELD_TYPE_UINT8)
		FIELD_MAP(linx_fd_info_t, lport, LINX_FIELD_TYPE_UINT8)
		FIELD_MAP(linx_fd_info_t, rport, LINX_FIELD_TYPE_UINT8)
		FIELD_MAP(linx_fd_info_t, l4port, LINX_FIELD_TYPE_CHARBUF)
	END_FIELD_MAPPINGS(fd)

	int ret = linx_hash_map_add_field_batch("fd", fd_mappings, fd_mappings_count);
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_add_field_batch failed");
    }

    return ret;
}

static int handle_regular_file(char *f_name, linx_fd_info_t *fd_info)
{
    char link_name[PROC_PATH_MAX_LEN] = {0};
    ssize_t read;

    read = readlink(f_name, link_name, PROC_PATH_MAX_LEN - 1);
    if (read <= 0) {
        return 0;
    }

    link_name[read] = '\0';

    if (LINX_FD_TYPE_UNSUPPORTED == fd_info->type) {
		if(0 == strcmp(link_name, "anon_inode:[eventfd]")) {
			fd_info->type = LINX_FD_TYPE_EVENT;
		} else if(0 == strcmp(link_name, "anon_inode:[signalfd]")) {
			fd_info->type = LINX_FD_TYPE_SIGNALFD;
		} else if(0 == strcmp(link_name, "anon_inode:[eventpoll]")) {
			fd_info->type = LINX_FD_TYPE_EVENTPOLL;
		} else if(0 == strcmp(link_name, "anon_inode:inotify")) {
			fd_info->type = LINX_FD_TYPE_INOTIFY;
		} else if(0 == strcmp(link_name, "anon_inode:[timerfd]")) {
			fd_info->type = LINX_FD_TYPE_TIMERFD;
		} else if(0 == strcmp(link_name, "anon_inode:[io_uring]")) {
			fd_info->type = LINX_FD_TYPE_IOURING;
		} else if(0 == strcmp(link_name, "anon_inode:[userfaultfd]")) {
			fd_info->type = LINX_FD_TYPE_USERFAULTFD;
		} else if(0 == strncmp(link_name, "anon_inode:[bpf", strlen("anon_inode:[bpf"))) {
			fd_info->type = LINX_FD_TYPE_BPF;
		} else if(0 == strcmp(link_name, "anon_inode:[pidfd]")) {
			fd_info->type = LINX_FD_TYPE_PIDFD;
		}

		fd_info->filename[0] = '\0';
    } else if (LINX_FD_TYPE_FILE_V2 == fd_info->type) {
		if (0 == strncmp(link_name, "/memfd:", strlen("/memfd:"))) {
			fd_info->type = LINX_FD_TYPE_MEMFD;
			strlcpy(fd_info->filename, link_name, sizeof(fd_info->filename));
		} else {
			strlcpy(fd_info->name, link_name, sizeof(fd_info->name));
		}
	} else {
		strlcpy(fd_info->filename, link_name, sizeof(fd_info->filename));
	}

    return 0;
}

static int handle_file(char *f_name, struct stat *sb, 
					   uint64_t net_ns, pid_t pid, 
                       linx_fd_info_t *fd_info)
{
    int ret;

    switch (sb->st_mode & __S_IFMT) {
        case __S_IFIFO:
            break;
        case __S_IFREG:
        case __S_IFBLK:
        case __S_IFCHR:
        case __S_IFLNK:
            fd_info->type = LINX_FD_TYPE_FILE_V2;
            ret = handle_regular_file(f_name, fd_info);
            break;
        case __S_IFDIR:
            break;
        case __S_IFSOCK:
            break;
        default:
			fd_info->type = LINX_FD_TYPE_UNSUPPORTED;
			ret = handle_regular_file(f_name, fd_info);
            break;
    }

    return ret;
}

linx_fd_info_t *linx_fd_info_create(pid_t pid, int64_t fd)
{
    struct stat sb;
    uint64_t net_ns;
	char path[PROC_PATH_MAX_LEN];
	linx_fd_info_t *fd_info = NULL;

    snprintf(path, PROC_PATH_MAX_LEN, "/proc/%d/ns/net", pid);
    if (stat(path, &sb) == -1) {
        net_ns = 0;
    } else {
        net_ns = sb.st_ino;
    }

	snprintf(path, PROC_PATH_MAX_LEN, "/proc/%d/fd/%ld", pid, fd);

	if (-1 == stat(path, &sb)) {
		return NULL;	
    }

	fd_info = calloc(1, sizeof(fd_info));
	if (!fd_info) {
		return NULL;
	}

	fd_info->num = fd;

	handle_file(path, &sb, net_ns, pid, fd_info);

    return fd_info;
}

void linx_fd_info_destroy(linx_fd_info_t *fd_info)
{
	if (!fd_info) {
		return;
	}

	free(fd_info);
	fd_info = NULL;
}

void linx_fd_info_cleanup(linx_fd_info_t *fdlist)
{
	linx_fd_info_t *fd_info, *tmp;
	HASH_ITER(hh, fdlist, fd_info, tmp) {
		HASH_DEL(fdlist, fd_info);
		linx_fd_info_destroy(fd_info);
	}
}

const char *linx_fd_type_sting_get(linx_fd_type_t f_type)
{
    if (f_type < LINX_FD_TYPE_UNKNOWN || f_type >= LINX_FD_TYPE_MAX) {
        return "<NA>";
    }

    return s_linx_fd_type_string[f_type];
}
