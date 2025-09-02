#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>

#include "linx_log.h"
#include "linx_fd_info.h"
#include "linx_hash_map.h"
#include "linx_machine_status.h"
#include "linx_process_cache_define.h"

#define SOCKET_SCAN_BUFFER_SIZE 1024 * 1024

static const linx_fd_type_str_t s_linx_fd_type_string[LINX_FD_TYPE_MAX] = {
    [LINX_FD_TYPE_UNKNOWN] = {"<NA>", 'O'},
	[LINX_FD_TYPE_FILE] = {"file", 'f'},
	[LINX_FD_TYPE_DIRECTORY] = {"directory", 'd'},
	[LINX_FD_TYPE_IPV4_SOCK] = {"ipv4", '4'},
	[LINX_FD_TYPE_IPV6_SOCK] = {"ipv6", '6'},
	[LINX_FD_TYPE_IPV4_SERVSOCK] = {"ipv4", '4'},
	[LINX_FD_TYPE_IPV6_SERVSOCK] = {"ipv6", '6'},
	[LINX_FD_TYPE_FIFO] = {"pipe", 'p'},
	[LINX_FD_TYPE_UNIX_SOCK] = {"unix", 'u'},
	[LINX_FD_TYPE_EVENT] = {"event", 'e'},
	[LINX_FD_TYPE_UNSUPPORTED] = {"<NA>", 'U'},
	[LINX_FD_TYPE_SIGNALFD] = {"signalfd", 's'},
	[LINX_FD_TYPE_EVENTPOLL] = {"eventpoll", 'l'},
	[LINX_FD_TYPE_INOTIFY] = {"inotify", 'i'},
	[LINX_FD_TYPE_TIMERFD] = {"timerfd", 't'},
	[LINX_FD_TYPE_NETLINK] = {"netlink", 'n'},
	[LINX_FD_TYPE_FILE_V2] = {"file", 'f'},
	[LINX_FD_TYPE_BPF] = {"bpf", 'b'},
	[LINX_FD_TYPE_USERFAULTFD] = {"userfaultfd", 'a'},
	[LINX_FD_TYPE_IOURING] = {"io_uring", 'r'},
	[LINX_FD_TYPE_MEMFD] = {"memfd", 'm'},
	[LINX_FD_TYPE_PIDFD] = {"pidfd", 'P'},
};

static const char *s_linx_l4proto_string[LINX_PROTO_MAX] = {
	[LINX_PROTO_UNKNOWN] = "unknown",
    [LINX_PROTO_NA] = "<NA>",
    [LINX_PROTO_TCP] = "tcp",
    [LINX_PROTO_UDP] = "udp",
    [LINX_PROTO_ICMP] = "icmp",
    [LINX_PROTO_RAW] = "raw",
};

int linx_fd_info_bind_field(void)
{
	BEGIN_FIELD_MAPPINGS(fd)
		FIELD_MAP(linx_fd_info_t, num, LINX_FIELD_TYPE_INT64)
		FIELD_MAP(linx_fd_info_t, type, LINX_FIELD_TYPE_CHARBUF_ARRAY)
		FIELD_MAP(linx_fd_info_t, typechar, LINX_FIELD_TYPE_CHARBUF_ARRAY)
		FIELD_MAP(linx_fd_info_t, name, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, directory, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, filename, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, ip, LINX_FIELD_TYPE_CHARBUF_ARRAY)
		FIELD_MAP(linx_fd_info_t, cip, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, sip, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, lip, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, rip, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, net, LINX_FIELD_TYPE_CHARBUF_ARRAY)
		FIELD_MAP(linx_fd_info_t, cnet, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, snet, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, lnet, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, rnet, LINX_FIELD_TYPE_CHARBUF)
		FIELD_MAP(linx_fd_info_t, port, LINX_FIELD_TYPE_UINT16)
		FIELD_MAP(linx_fd_info_t, cport, LINX_FIELD_TYPE_UINT16)
		FIELD_MAP(linx_fd_info_t, sport, LINX_FIELD_TYPE_UINT16)
		FIELD_MAP(linx_fd_info_t, lport, LINX_FIELD_TYPE_UINT16)
		FIELD_MAP(linx_fd_info_t, rport, LINX_FIELD_TYPE_UINT16)
		FIELD_MAP(linx_fd_info_t, l4proto, LINX_FIELD_TYPE_CHARBUF_ARRAY)
	END_FIELD_MAPPINGS(fd)

	int ret = linx_hash_map_add_field_batch("fd", fd_mappings, fd_mappings_count);
    if (ret) {
        LINX_LOG_ERROR("linx_hash_map_add_field_batch failed");
    }

    return ret;
}

static int convert_hex_ip(char *hex_addr, char *output, size_t size)
{
	size_t len;
	uint32_t ip_num[4];

	if (hex_addr == NULL) {
		output[0] = '\0';
		return -1;
	}

	if (output == NULL) {
		return -1;
	}

	len = strlen(hex_addr);

	if (len == 8) {
		if (sscanf(hex_addr, "%x", &ip_num[0]) != 1) {
			output[0] = '\0';
			return -1;
		}

		if (inet_ntop(AF_INET, &ip_num[0], output, size) == NULL) {
			output[0] = '\0';
			return -1;
		}

		return 0;
	} else if (len == 32) {
		for (int i = 0; i < 4; ++i) {
			char buf[9];
			memcpy(buf, hex_addr + i * 8, 8);
			buf[8] = '\0';

			if (sscanf(buf, "%x", &ip_num[i]) != 1) {
				output[0] = '\0';
				return -1;
			}
		}

		if (inet_ntop(AF_INET6, ip_num, output, size) == NULL) {
			output[0] = '\0';
			return -1;
		}

		return 0;
	}

	return -1;
}

static bool fd_is_ipv6_server_socket(char *ip6_addr)
{
	if (ip6_addr == NULL || *ip6_addr == '\0') {
		return false;
	}

	while (*ip6_addr != '\0') {
		if (*ip6_addr != '0') {
			return false;
		}

		ip6_addr++;
	}

	return true;
}

static int read_ipv4_sockets_from_proc_fd(char *dir, linx_fd_info_t **sockets, linx_proto_type_t l4proto, char *net_mask)
{
	FILE *f;
	uint32_t rsize, j;
	char *scan_buf, *scan_pos, *tmp_pos, *end, tc;

	scan_buf = (char *)malloc(SOCKET_SCAN_BUFFER_SIZE);
	if (scan_buf == NULL) {
		return -1;
	}

	f = fopen(dir, "r");
	if (f == NULL) {
		free(scan_buf);
		return -1;
	}

	while ((rsize = fread(scan_buf, 1, SOCKET_SCAN_BUFFER_SIZE, f)) != 0) {
		char *scan_end = scan_buf + rsize;
		scan_pos = scan_buf;

		while (scan_pos <= scan_end) {
			scan_pos = memchr(scan_pos, '\n', scan_end - scan_pos);
			if (scan_pos == NULL) {
				break;
			}

			linx_fd_info_t *fd_info = calloc(1, sizeof(linx_fd_info_t));
			if (fd_info == NULL) {
				fclose(f);
				free(scan_buf);
				return -1;
			}

			/**
			 * 跳过sl序号列
			 */
			scan_pos = memchr(scan_pos, ':', scan_end - scan_pos);
			if (scan_pos == NULL) {
				free(fd_info);
				break;
			}

			scan_pos += 2;
			if (scan_pos + 80 >= scan_end) {
				free(fd_info);
				break;
			}

			/**
			 * 扫描 local_address
			 * 8位IP:4位port
			 */
			tc = *(scan_pos + 8);
			*(scan_pos + 8) = 0;
			convert_hex_ip(scan_pos, fd_info->sip, sizeof(fd_info->sip));
			*(scan_pos + 8) = tc;

			scan_pos += 9;
			tc = *(scan_pos + 4);
			*(scan_pos + 4) = 0;
			fd_info->sport = (uint16_t)strtoul(scan_pos, &end, 16);
			*(scan_pos + 4) = tc;

			scan_pos += 5;

			/**
			 * 扫描remote_address
			 * 8位IP:4位port
			 */
			tc = *(scan_pos + 8);
			*(scan_pos + 8) = 0;
			convert_hex_ip(scan_pos, fd_info->rip, sizeof(fd_info->rip));
			*(scan_pos + 8) = tc;

			scan_pos += 9;
			tc = *(scan_pos + 4);
			*(scan_pos + 4) = 0;
			fd_info->rport = (uint16_t)strtoul(scan_pos, &end, 16);
			*(scan_pos + 4) = tc;

			scan_pos += 4;

			/**
			 * 扫描文件 inode
			 */
			for (j = 0; j < 6; ++j) {
				scan_pos++;

				scan_pos = memchr(scan_pos, ' ', scan_end - scan_pos);
				if (scan_pos == NULL) {
					break;
				}

				while (scan_pos < scan_end && *scan_pos == ' ') {
					scan_pos++;
				}
				
				if (scan_pos >= scan_end) {
					break;
				}
			}

			if (j < 6) {
				free(fd_info);
				break;
			}

			tmp_pos = scan_pos;
			scan_pos = memchr(scan_pos, ' ', scan_end - scan_pos);
			if (scan_pos == NULL || scan_pos >= scan_end) {
				free(fd_info);
				break;
			}

			tc = *(scan_pos);

			fd_info->ino = (uint64_t)strtoull(tmp_pos, &end, 10);

			*(scan_pos) = tc;

			if (fd_info->rip[0] == 0) {
				fd_info->type.num = LINX_FD_TYPE_IPV4_SERVSOCK;
				fd_info->l4proto = linx_proto_type_string_get(l4proto);
				fd_info->port = fd_info->sport;
				fd_info->ip = fd_info->sip;
				snprintf(fd_info->snet, sizeof(fd_info->snet), "%s%s", fd_info->ip, net_mask);
				fd_info->net = fd_info->snet;
			} else {
				fd_info->type.num = LINX_FD_TYPE_IPV4_SOCK;
				fd_info->l4proto = linx_proto_type_string_get(l4proto);
			}

			HASH_ADD_INT64((*sockets), ino, fd_info);

			scan_pos++;
		}
	}

	fclose(f);
	free(scan_buf);
	return 0;
}

static int read_ipv6_sockets_from_proc_fd(char *dir, linx_fd_info_t **sockets, linx_proto_type_t l4proto, char *net_mask)
{
	FILE *f;
	uint32_t rsize, j;
	char *scan_buf, *scan_pos, *tmp_pos, *end, tc;
	
	scan_buf = (char *)malloc(SOCKET_SCAN_BUFFER_SIZE);
	if (scan_buf == NULL) {
		return -1;
	}

	f = fopen(dir, "r");
	if (NULL == f) {
		free(scan_buf);
		return -1;
	}

	while ((rsize = fread(scan_buf, 1, SOCKET_SCAN_BUFFER_SIZE, f)) != 0) {
		char *scan_end = scan_buf + rsize;
		scan_pos = scan_buf;

		while (scan_pos <= scan_end) {
			scan_pos = memchr(scan_pos, '\n', scan_end - scan_pos);
			if (scan_pos == NULL) {
				break;
			}

			linx_fd_info_t *fd_info = calloc(1, sizeof(linx_fd_info_t));
			if(fd_info == NULL) {
				fclose(f);
				free(scan_buf);
				return -1;
			}

			/**
			 * 跳过sl序号列
			 */
			scan_pos = memchr(scan_pos, ':', scan_end - scan_pos);
			if (scan_pos == NULL) {
				free(fd_info);
				break;
			}

			scan_pos += 2;
			if (scan_pos + 80 >= scan_end) {
				free(fd_info);
				break;
			}

			/**
			 * 扫描 local_address
			 * 32位IP:4位port
			 */
			tc = *(scan_pos + 32);
			*(scan_pos + 32) = 0;
			convert_hex_ip(scan_pos, fd_info->sip, sizeof(fd_info->sip));
			*(scan_pos + 32) = tc;
			scan_pos += 33;

			tc = *(scan_pos + 4);
			*(scan_pos + 4) = 0;
			fd_info->sport = (uint16_t)strtoul(scan_pos, &end, 16);
			*(scan_pos + 4) = tc;

			scan_pos += 5;

			/**
			 * 扫描remote_address
			 * 32位IP:4位port
			 */
			tc = *(scan_pos + 32);
			*(scan_pos + 32) = 0;
			convert_hex_ip(scan_pos, fd_info->rip, sizeof(fd_info->rip));
			*(scan_pos + 32) = tc;
			scan_pos += 33;

			tc = *(scan_pos + 4);
			*(scan_pos + 4) = 0;
			fd_info->rport = (uint16_t)strtoul(scan_pos, &end, 16);
			*(scan_pos + 4) = tc;

			scan_pos += 4;

			/**
			 * 扫描文件 inode
			 */
			for (j = 0; j < 6; ++j) {
				scan_pos++;

				scan_pos = memchr(scan_pos, ' ', scan_end - scan_pos);
				if (scan_pos == NULL) {
					break;
				}

				while (scan_pos < scan_end && *scan_pos == ' ') {
					scan_pos++;
				}
				
				if (scan_pos >= scan_end) {
					break;
				}
			}

			if (j < 6) {
				free(fd_info);
				break;
			}

			tmp_pos = scan_pos;
			scan_pos = memchr(scan_pos, ' ', scan_end - scan_pos);
			if (scan_pos == NULL || scan_pos >= scan_end) {
				free(fd_info);
				break;
			}

			tc = *(scan_pos);

			fd_info->ino = (uint64_t)strtoull(tmp_pos, &end, 10);

			*(scan_pos) = tc;

			if (fd_is_ipv6_server_socket(fd_info->rip)) {
				fd_info->type.num = LINX_FD_TYPE_IPV6_SERVSOCK;
				fd_info->l4proto = linx_proto_type_string_get(l4proto);
				fd_info->port = fd_info->sport;
				fd_info->ip = fd_info->sip;
				snprintf(fd_info->snet, sizeof(fd_info->snet), "%s%s", fd_info->ip, net_mask);
				fd_info->net = fd_info->snet;
			} else {
				fd_info->type.num = LINX_FD_TYPE_IPV6_SOCK;
				fd_info->l4proto = linx_proto_type_string_get(l4proto);
			}

			HASH_ADD_INT64((*sockets), ino, fd_info);

			scan_pos++;
		}
	}

	fclose(f);
	free(scan_buf);
	return 0;
}

static int read_unix_sockets_from_proc_fd(char *filename, linx_fd_info_t **sockets)
{
	FILE *f;
	char line[PROC_PATH_MAX_LEN];
	int first_line = false;
	char *delimiters = " \t";
	char *token;

	f = fopen(filename, "r");
	if (NULL == f) {
		return -1;
	}

	while (NULL != fgets(line, sizeof(line), f)) {
		char *scratch;

		if(!first_line) {
			first_line = true;
			continue;
		}

		linx_fd_info_t *fd_info = calloc(1, sizeof(linx_fd_info_t));
		if (fd_info == NULL) {
			fclose(f);
			return -1;
		}

		fd_info->type.num = LINX_FD_TYPE_UNIX_SOCK;

		/**
		 * 1. Num
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * TODO: 是否需要unix的endpoint
		 */

		/**
		 * 2. RefCount
		 */
		token = strtok_r(NULL, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 3. Protocol
		 */
		token = strtok_r(NULL, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 4. Flags
		 */
		token = strtok_r(NULL, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 5. Type
		 */
		token = strtok_r(NULL, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 6. St
		 */
		token = strtok_r(NULL, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 7. Inode
		 */
		token = strtok_r(NULL, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		sscanf(token, "%lu", &(fd_info->ino));

		/**
		 * 8. Path
		 */
		token = strtok_r(NULL, delimiters, &scratch);
		if(token != NULL) {
			strlcpy(fd_info->name, token, sizeof(fd_info->name));
		} else {
			fd_info->name[0] = '\0';
		}

		HASH_ADD_INT64((*sockets), ino, fd_info);
	}

	fclose(f);
	return 0;
}

static int read_netlink_sockets_from_proc_fd(char *filename, linx_fd_info_t **sockets)
{
	FILE *f;
	char line[PROC_PATH_MAX_LEN];
	int first_line = false;
	char *delimiters = " \t";
	char *token;

	f = fopen(filename, "r");
	if(NULL == f) {
		return -1;
	}

	while(NULL != fgets(line, sizeof(line), f)) {
		char *scratch;

		if(!first_line) {
			first_line = true;
			continue;
		}

		linx_fd_info_t *fd_info = calloc(1, sizeof(linx_fd_info_t));
		if(fd_info == NULL) {
			fclose(f);
			return 1;
		}

		fd_info->type.num = LINX_FD_TYPE_UNIX_SOCK;

		/**
		 * 1. Num
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 2. Eth
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 3. Pid
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 4. Groups
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 5. Rmem
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 6. Wmem
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 7. Dump
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 8. Locks
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 9. Drops
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		/**
		 * 10. Inode
		 */
		token = strtok_r(line, delimiters, &scratch);
		if(token == NULL) {
			free(fd_info);
			continue;
		}

		sscanf(token, "%lu", &(fd_info->ino));

		HASH_ADD_INT64((*sockets), ino, fd_info);
	}

	fclose(f);
	return 0;
}

static int handle_pipe_file(char *f_name, linx_fd_info_t *fd_info)
{
	char link_name[PROC_PATH_MAX_LEN];
	ssize_t r;
	uint64_t ino;
	struct stat sb;

	r = readlink(f_name, link_name, sizeof(link_name) - 1);
	if (r <= 0) {
		return -1;
	}

	link_name[r] = '\0';
	if (1 != sscanf(link_name, "pipe:[%li]", &ino)) {
		if (-1 == stat(link_name, &sb)) {
			return 0;
		}

		ino = sb.st_ino;
	}

	strlcpy(fd_info->filename, link_name, sizeof(fd_info->filename));
	fd_info->ino = ino;

	return 0;
}

static int handle_regular_file(char *procdir, char *f_name, linx_fd_info_t *fd_info)
{
	(void)procdir;

    char link_name[PROC_PATH_MAX_LEN] = {0};
    ssize_t read;

    read = readlink(f_name, link_name, PROC_PATH_MAX_LEN - 1);
    if (read <= 0) {
        return 0;
    }

    link_name[read] = '\0';

    if (LINX_FD_TYPE_UNSUPPORTED == fd_info->type.num) {
		if(0 == strcmp(link_name, "anon_inode:[eventfd]")) {
			fd_info->type.num = LINX_FD_TYPE_EVENT;
		} else if(0 == strcmp(link_name, "anon_inode:[signalfd]")) {
			fd_info->type.num = LINX_FD_TYPE_SIGNALFD;
		} else if(0 == strcmp(link_name, "anon_inode:[eventpoll]")) {
			fd_info->type.num = LINX_FD_TYPE_EVENTPOLL;
		} else if(0 == strcmp(link_name, "anon_inode:inotify")) {
			fd_info->type.num = LINX_FD_TYPE_INOTIFY;
		} else if(0 == strcmp(link_name, "anon_inode:[timerfd]")) {
			fd_info->type.num = LINX_FD_TYPE_TIMERFD;
		} else if(0 == strcmp(link_name, "anon_inode:[io_uring]")) {
			fd_info->type.num = LINX_FD_TYPE_IOURING;
		} else if(0 == strcmp(link_name, "anon_inode:[userfaultfd]")) {
			fd_info->type.num = LINX_FD_TYPE_USERFAULTFD;
		} else if(0 == strncmp(link_name, "anon_inode:[bpf", strlen("anon_inode:[bpf"))) {
			fd_info->type.num = LINX_FD_TYPE_BPF;
		} else if(0 == strcmp(link_name, "anon_inode:[pidfd]")) {
			fd_info->type.num = LINX_FD_TYPE_PIDFD;
		}

		fd_info->filename[0] = '\0';
    } else if (LINX_FD_TYPE_FILE_V2 == fd_info->type.num) {
		if (0 == strncmp(link_name, "/memfd:", strlen("/memfd:"))) {
			fd_info->type.num = LINX_FD_TYPE_MEMFD;
			strlcpy(fd_info->filename, link_name, sizeof(fd_info->filename));
		} else {
			strlcpy(fd_info->name, link_name, sizeof(fd_info->name));
		}
	} else {
		strlcpy(fd_info->filename, link_name, sizeof(fd_info->filename));
	}

    return 0;
}

static int fd_read_sockets(char *procdir, linx_fd_socket_list_t *sockets, char *net_mask)
{
	char file_name[PROC_PATH_MAX_LEN];
	char net_root[128];

	if (sockets->net_ns) {
		snprintf(net_root, sizeof(net_root), "%snet/", procdir);
	} else {
		snprintf(net_root, sizeof(net_root), "/proc/net/");
	}

	snprintf(file_name, sizeof(file_name), "%stcp", net_root);
	if (read_ipv4_sockets_from_proc_fd(file_name, &sockets->sockets, LINX_PROTO_TCP, net_mask)) {
		linx_fd_info_cleanup(sockets->sockets);
		return -1;
	}

	snprintf(file_name, sizeof(file_name), "%sudp", net_root);
	if (read_ipv4_sockets_from_proc_fd(file_name, &sockets->sockets, LINX_PROTO_UDP, net_mask)) {
		linx_fd_info_cleanup(sockets->sockets);
		return -1;
	}

	snprintf(file_name, sizeof(file_name), "%sraw", net_root);
	if (read_ipv4_sockets_from_proc_fd(file_name, &sockets->sockets, LINX_PROTO_RAW, net_mask)) {
		linx_fd_info_cleanup(sockets->sockets);
		return -1;
	}

	snprintf(file_name, sizeof(file_name), "%sunix", net_root);
	if (read_unix_sockets_from_proc_fd(file_name, &sockets->sockets)) {
		linx_fd_info_cleanup(sockets->sockets);
		return -1;
	}

	snprintf(file_name, sizeof(file_name), "%snetlink", net_root);
	if (read_netlink_sockets_from_proc_fd(file_name, &sockets->sockets)) {
		linx_fd_info_cleanup(sockets->sockets);
		return -1;
	}

	snprintf(file_name, sizeof(file_name), "%stcp6", net_root);
	if (access(file_name, R_OK) == 0) {
		if (read_ipv6_sockets_from_proc_fd(file_name, &sockets->sockets, LINX_PROTO_TCP, net_mask)) {
			linx_fd_info_cleanup(sockets->sockets);
			return -1;
		}

		snprintf(file_name, sizeof(file_name), "%sudp6", net_root);
		if (read_ipv6_sockets_from_proc_fd(file_name, &sockets->sockets, LINX_PROTO_UDP, net_mask)) {
			linx_fd_info_cleanup(sockets->sockets);
			return -1;
		}

		snprintf(file_name, sizeof(file_name), "%sraw6", net_root);
		if (read_ipv6_sockets_from_proc_fd(file_name, &sockets->sockets, LINX_PROTO_RAW, net_mask)) {
			linx_fd_info_cleanup(sockets->sockets);
			return -1;
		}
	}

	return 0;
}

static int handle_socket_file(char *procdir, char *f_name, linx_fd_info_t *fd_info, 
							  uint64_t net_ns, linx_fd_socket_list_t **sockets_by_ns)
{
	ssize_t read;
	char link_name[PROC_PATH_MAX_LEN];
	uint64_t ino;
	linx_fd_socket_list_t *sockets = NULL;
	linx_fd_info_t *fdi;

	if (*sockets_by_ns == (void *)-1) {
		return 0;
	} else {
		HASH_FIND_INT64(*sockets_by_ns, &net_ns, sockets);
		if (sockets == NULL) {
			sockets = calloc(1, sizeof(linx_fd_socket_list_t));
			if (sockets == NULL) {
				LINX_LOG_ERROR("sockets alloc failed!");
				return -1;
			}

			sockets->net_ns = net_ns;
			sockets->sockets = NULL;

			HASH_ADD_INT64(*sockets_by_ns, net_ns, sockets);

			if (fd_read_sockets(procdir, sockets, linx_machine_status_get_ifinfo()->v4list[0].net_mask)) {
				sockets->sockets = NULL;
				LINX_LOG_DEBUG("Cannot read sockets!");
				return -1;
			}
		}
	}

    read = readlink(f_name, link_name, PROC_PATH_MAX_LEN - 1);
    if (read <= 0) {
        return 0;
    }

    link_name[read] = '\0';

	strlcpy(fd_info->name, link_name, sizeof(fd_info->name));

	if (1 != sscanf(link_name, "socket:[%li]", &ino)) {
		fd_info->type.num = LINX_FD_TYPE_UNSUPPORTED;
		return 0;
	}

	HASH_FIND_INT64(sockets->sockets, &ino, fdi);
	if (fdi != NULL) {
		memcpy(fd_info, fdi, sizeof(linx_fd_info_t));
		fd_info->ino = ino;
		fd_info->type = fdi->type;
	}

	return 0;
}

static int handle_file(char *procdir, char *f_name, 
					   struct stat *sb, uint64_t net_ns, 
					   pid_t pid, linx_fd_info_t *fd_info, 
					   linx_fd_socket_list_t **sockets)
{
	(void)net_ns;
	(void)pid;

    int ret;

    switch (sb->st_mode & __S_IFMT) {
        case __S_IFIFO:
			fd_info->type.num = LINX_FD_TYPE_FIFO;
			handle_pipe_file(f_name, fd_info);
            break;
        case __S_IFREG:
        case __S_IFBLK:
        case __S_IFCHR:
        case __S_IFLNK:
            fd_info->type.num = LINX_FD_TYPE_FILE_V2;
			fd_info->ino = sb->st_ino;
            ret = handle_regular_file(procdir, f_name, fd_info);
            break;
        case __S_IFDIR:
			fd_info->type.num = LINX_FD_TYPE_DIRECTORY;
			fd_info->ino = sb->st_ino;
			ret = handle_regular_file(procdir, f_name, fd_info);
            break;
        case __S_IFSOCK:
			fd_info->type.num = LINX_FD_TYPE_UNKNOWN;
			ret = handle_socket_file(procdir, f_name, fd_info, net_ns, sockets);
            break;
        default:
			fd_info->type.num = LINX_FD_TYPE_UNSUPPORTED;
			fd_info->ino = sb->st_ino;
			ret = handle_regular_file(procdir, f_name, fd_info);
            break;
    }

	if (!fd_info->l4proto) {
		fd_info->l4proto = linx_proto_type_string_get(LINX_PROTO_UNKNOWN);
	}

	fd_info->type.str = linx_fd_type_string_get(fd_info->type.num);
	fd_info->typechar = linx_fd_type_str_get(fd_info->type.num);

    return ret;
}

linx_fd_info_t *linx_fd_info_create(pid_t pid, int64_t fd)
{
    struct stat sb;
    uint64_t net_ns;
	char path[PROC_PATH_MAX_LEN];
	char procdir[128];
	linx_fd_info_t *fd_info = NULL;
	linx_fd_socket_list_t *sockets = NULL;

	snprintf(procdir, sizeof(procdir), "/proc/%d/", pid);
    snprintf(path, PROC_PATH_MAX_LEN, "%sns/net", procdir);
    if (stat(path, &sb) == -1) {
        net_ns = 0;
    } else {
        net_ns = sb.st_ino;
    }

	snprintf(path, PROC_PATH_MAX_LEN, "%sfd/%ld", procdir, fd);

	if (-1 == stat(path, &sb)) {
		return NULL;	
    }

	fd_info = calloc(1, sizeof(linx_fd_info_t));
	if (!fd_info) {
		return NULL;
	}

	fd_info->num = fd;

	if (handle_file(procdir, path, &sb, net_ns, pid, fd_info, &sockets)) {
		free(fd_info);
		fd_info = NULL;
	}

	if (sockets != NULL) {
		linx_fd_socket_list_free(&sockets);
	}

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

void linx_fd_socket_list_free(linx_fd_socket_list_t **list)
{
	linx_fd_socket_list_t *fd, *tfd;

	if (*list) {
		HASH_ITER(hh, *list, fd, tfd) {
			HASH_DEL(*list, fd);
			linx_fd_info_cleanup(fd->sockets);
			free(fd);
		}

		*list = NULL;
	}
}

char *linx_fd_type_string_get(linx_fd_type_t f_type)
{
    if (f_type < LINX_FD_TYPE_UNKNOWN || f_type >= LINX_FD_TYPE_MAX) {
        return "<NA>";
    }

    return s_linx_fd_type_string[f_type].string;
}

char linx_fd_type_str_get(linx_fd_type_t f_type)
{
	if (f_type < LINX_FD_TYPE_UNKNOWN || f_type >= LINX_FD_TYPE_MAX) {
        return 'O';
    }

    return s_linx_fd_type_string[f_type].str;
}

char *linx_proto_type_string_get(linx_proto_type_t type)
{
	if (type < 0 || type >= LINX_PROTO_MAX) {
		return "<NA>";
	}

	return (char *)s_linx_l4proto_string[type];
}
