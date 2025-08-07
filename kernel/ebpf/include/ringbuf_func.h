#ifndef __RINGBUF_FUNC_H__
#define __RINGBUF_FUNC_H__

#include "maps.h"
#include "linx_event.h"
#include "linx_event_type.h"
#include "struct_define.h"
#include "extract_from_kernel.h"

typedef enum {
	USER = 0,
	KERNEL = 1,
} read_memory_t;

typedef enum {
    OUTBOUND = 0,
    INBOUND = 1,
} connection_direction_t;

typedef struct {
    bool only_port_range;
    linx_event_type_t type;
    long mmsg_index;
    unsigned long *mm_args;
} snaplen_args_t;

#define FILE_PATH_MAX_DEPTH     (12)

#define SAFE_ACCESS(x) ((x) & (LINX_EVENT_MAX_SIZE - 1))

#define PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, value, type)                       \
    do {                                                                        \
        *((type *)&ringbuf->data[SAFE_ACCESS(ringbuf->payload_pos)]) = value;   \
        ringbuf->payload_pos += sizeof(type);                                   \
    } while (0)

#define PUSH_FIXED_SIZE_TO_RINGBUF(ringbuf, size)                                       \
    do {                                                                                \
        ((linx_event_t *)ringbuf->data)->params_size[ringbuf->index++] = size;          \
    } while (0)

#define PUSH_FIXED_VALUE_AND_SIZE_TO_RINGBUF(ringbuf, value, type)              \
    PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, value, type);                          \
    PUSH_FIXED_SIZE_TO_RINGBUF(ringbuf, sizeof(type));

static inline void linx_push_previous_character(uint8_t *data, uint64_t *payload_pos, char c)
{
    *((char *)&data[SAFE_ACCESS(*payload_pos - 1)]) = c;
}

static inline uint64_t linx_get_ppid(struct task_struct *task)
{
    uint32_t ppid = 0;

    if (bpf_core_field_exists(task->parent)) {
        ppid = BPF_CORE_READ(task, parent, tgid);
    } else if (bpf_core_field_exists(task->real_parent)) {
        ppid = BPF_CORE_READ(task, real_parent, tgid);
    }

    return (uint64_t)ppid;
}

static inline struct mount *real_mount(struct vfsmount *mnt)
{
	return container_of(mnt, struct mount, mnt);
}

static inline long linx_get_file_path(struct file *file, char *buf, size_t size)
{
    struct dentry *dentry, *dentry_parent, *dentry_mnt;
    struct vfsmount *vfsmnt;
    struct mount *mnt, *mnt_parent;
    struct qstr components[FILE_PATH_MAX_DEPTH] = {}, qname;
    long ret, depth = 0;
    size_t pos = 0, avail;

    buf[0] = '\0';

    if (bpf_probe_read(&dentry, sizeof(dentry), &file->f_path.dentry) < 0 || !dentry) {
        return -1;
    }

    if (bpf_probe_read(&vfsmnt, sizeof(vfsmnt), &file->f_path.mnt) < 0 || !vfsmnt) {
        return -1;
    }

    mnt = container_of(vfsmnt, struct mount, mnt);
    if (!mnt) {
        return -1;
    }

    #pragma unroll
    for (int i = 0; i < FILE_PATH_MAX_DEPTH; ++i) {
        if (bpf_probe_read(&mnt_parent, sizeof(mnt_parent), &mnt->mnt_parent) < 0 || !mnt_parent) {
            break;
        }

        if (bpf_probe_read(&dentry_mnt, sizeof(dentry_mnt), &vfsmnt->mnt_root) < 0 || !dentry_mnt) {
            break;
        }

        if (bpf_core_read(&dentry_parent, sizeof(dentry_parent), &dentry->d_parent) < 0 || !dentry_parent) {
            break;
        }

        if (dentry == dentry_mnt || dentry == dentry_parent) {
            if (dentry != dentry_mnt || mnt != mnt_parent) {
                if (bpf_probe_read(&dentry, sizeof(dentry), &mnt->mnt_mountpoint) < 0 || !dentry) {
                    break;
                }

                if (bpf_probe_read(&mnt, sizeof(mnt), &mnt->mnt_parent) < 0 || !mnt_parent) {
                    break;
                }

                if (bpf_probe_read(&vfsmnt, sizeof(vfsmnt), &mnt->mnt) < 0 || !vfsmnt) {
                    break;
                }

                continue;
            }

            break;
        }

        if (bpf_probe_read(&qname, sizeof(qname), &dentry->d_name) < 0) {
            break;
        }

        if (qname.len > 0 && depth < FILE_PATH_MAX_DEPTH) {
            components[depth].name = qname.name;
            components[depth].len = qname.len;
            depth++;
        }

        dentry = dentry_parent;
    }

    if (depth == 0) {
        return -1;
    }

    for (int i = depth - 1; i >= 0; --i) {
        if (pos >= size - 1) {
            break;
        }

        buf[pos++] = '/';

        avail = size - pos;
        if (avail <= 0) {
            break;
        }
    
        ret = bpf_probe_read_kernel_str(buf + pos, avail, components[i].name);
        if (ret < 0){
            break;
        }
    
        pos += (ret > 0) ? ret - 1 : 0;
    }

    if (pos < size) {
        buf[pos] = '\0';
    } else if (size > 0) {
        buf[size - 1] = '\0';
    }

    return (pos < size) ? pos : size - 1;
}

static inline void linx_get_task_file(struct task_struct *task, linx_event_t *event)
{
    unsigned int max_fds = 0;
    struct file **files, *file;

    max_fds = BPF_CORE_READ(task, files, fdt, max_fds);
    if (!max_fds) {
        return;
    }

    files = BPF_CORE_READ(task, files, fdt, fd);
    if (!files) {
        return;
    }

    max_fds = max_fds < LINX_FDS_MAX_SIZE ? max_fds : LINX_FDS_MAX_SIZE;

    event->nfds = 0;
    for (int i = 0; i < max_fds; ++i) {
        if (bpf_probe_read(&file, sizeof(file), &files[i]) < 0 || !file) {
            continue;
        }
    
        if (event->nfds > LINX_FDS_MAX_SIZE - 1) {
            break;
        }

        event->fds[event->nfds] = i;
        
        linx_get_file_path(file, &event->fd_path[event->nfds++][0], LINX_PATH_MAX_SIZE);
    }
}

static inline int linx_get_process_cmdline(struct task_struct *task, char *cmdline)
{
    long ret = 0;
    struct mm_struct *mm = BPF_CORE_READ(task, mm);
    if (!mm) {
        return -1;
    }
    
    unsigned long arg_start = BPF_CORE_READ(mm, arg_start);
    unsigned long arg_end = BPF_CORE_READ(mm, arg_end);
    unsigned long len = arg_end - arg_start;

    if (len <= 0 || len > LINX_CMDLINE_MAX_SIZE - 1) {
        return -1;
    }

    ret = bpf_probe_read_user(cmdline, len, (void *)arg_start);

    for (int i = 0 ; i < len; ++i) {
        if (cmdline[i] == '\0') {
            cmdline[i] = ' ';
        }
    }

    cmdline[len - 1] = '\0';

    return ret;
}

static inline int linx_get_comm_fullpath(struct task_struct *task, char *fullpath)
{
    struct file *file = BPF_CORE_READ(task, mm, exe_file);
    if (!file) {
        return -1;
    }

    return (int)linx_get_file_path(file, fullpath, LINX_PATH_MAX_SIZE);
}

static inline int linx_get_parent_fullpath(struct task_struct *task, char *fullpath)
{
    char *comm = BPF_CORE_READ(task, parent, comm);
    if (!comm) {
        return -1;
    }

    return bpf_probe_read_kernel_str(fullpath, LINX_PATH_MAX_SIZE, comm);

    // return (int)linx_get_file_path(file, fullpath, LINX_PATH_MAX_SIZE);
}

static inline linx_ringbuf_t *linx_ringbuf_get(void)
{
    uint32_t cpuid = (uint32_t)bpf_get_smp_processor_id();
    return (linx_ringbuf_t *)bpf_map_lookup_elem(&linx_ringbuf_maps, &cpuid);
}

static inline void linx_ringbuf_load_event(linx_ringbuf_t *ringbuf, linx_event_type_t type, long res)
{
    struct task_struct *task = (struct task_struct *)bpf_get_current_task_btf();
    linx_event_t *event = (linx_event_t *)ringbuf->data;
    uint64_t tg_pid = bpf_get_current_pid_tgid();
    uint64_t uid_gid = bpf_get_current_uid_gid();

    event->tid = (uint64_t)(tg_pid >> 32);
    event->pid = (uint64_t)((uint32_t)tg_pid);
    event->ppid = linx_get_ppid(task);
    event->uid = (uint64_t)((uint32_t)uid_gid);
    event->gid = (uint64_t)(uid_gid >> 32);
    event->time = g_boot_time + bpf_ktime_get_boot_ns();
    event->res = (uint64_t)res;
    event->type = (uint32_t)type;
    event->size = 0;

    bpf_get_current_comm(&event->comm, LINX_COMM_MAX_SIZE);
    // linx_get_task_file(task, event);
    // linx_get_process_cmdline(task, &event->cmdline[0]);
    // linx_get_comm_fullpath(task, &event->fullpath[0]);
    // linx_get_parent_fullpath(task , &event->p_fullpath[0]);

    ringbuf->index = 0;
    ringbuf->payload_pos = LINX_EVENT_HEADER_SIZE;
    ringbuf->reserved_event_size = LINX_EVENT_MAX_SIZE;
}

static inline void linx_ringbuf_submit_event(linx_ringbuf_t *ringbuf)
{
    if (ringbuf->payload_pos > LINX_EVENT_MAX_SIZE) {
        return;
    }

    ((linx_event_t *)ringbuf->data)->size = ringbuf->payload_pos;

    int err = bpf_ringbuf_output(&ringbuf_map, ringbuf->data, ringbuf->payload_pos, 0);
    if (err) {
        bpf_printk("bpf_ringbuf_output error!\n");
        return;
    }
}

static inline void linx_ringbuf_store_s8(linx_ringbuf_t *ringbuf, int8_t value)
{
	PUSH_FIXED_VALUE_AND_SIZE_TO_RINGBUF(ringbuf, value, int8_t);
}

static inline void linx_ringbuf_store_s16(linx_ringbuf_t *ringbuf, int16_t value)
{
    PUSH_FIXED_VALUE_AND_SIZE_TO_RINGBUF(ringbuf, value, int16_t);
}

static inline void linx_ringbuf_store_s32(linx_ringbuf_t *ringbuf, int32_t value)
{
	PUSH_FIXED_VALUE_AND_SIZE_TO_RINGBUF(ringbuf, value, int32_t);
}

static inline void linx_ringbuf_store_s64(linx_ringbuf_t *ringbuf, int64_t value)
{
	PUSH_FIXED_VALUE_AND_SIZE_TO_RINGBUF(ringbuf, value, int64_t);
}

static inline void linx_ringbuf_store_u8(linx_ringbuf_t *ringbuf, uint8_t value)
{
	PUSH_FIXED_VALUE_AND_SIZE_TO_RINGBUF(ringbuf, value, uint8_t);
}

static inline void linx_ringbuf_store_u16(linx_ringbuf_t *ringbuf, uint16_t value)
{
	PUSH_FIXED_VALUE_AND_SIZE_TO_RINGBUF(ringbuf, value, uint16_t);
}

static inline void linx_ringbuf_store_u32(linx_ringbuf_t *ringbuf, uint32_t value)
{
	PUSH_FIXED_VALUE_AND_SIZE_TO_RINGBUF(ringbuf, value, uint32_t);
}

static inline void linx_ringbuf_store_u64(linx_ringbuf_t *ringbuf, uint64_t value)
{
	PUSH_FIXED_VALUE_AND_SIZE_TO_RINGBUF(ringbuf, value, uint64_t);
}

static inline void linx_ringbuf_store_empty(linx_ringbuf_t *ringbuf)
{
    ((linx_event_t *)ringbuf->data)->params_size[ringbuf->index++] = 0;
}

static inline uint16_t linx_push_charpointer(uint8_t *data,
                                             uint64_t *payload_pos,
                                             unsigned long charpointer,
                                             uint16_t limit,
                                             read_memory_t mem)
{
    int written_bytes = 0;

    if (mem == KERNEL) {
        written_bytes = bpf_probe_read_kernel_str(&data[SAFE_ACCESS(*payload_pos)],
                                                limit,
                                                (char *)charpointer);    
    } else {
        written_bytes = bpf_probe_read_user_str(&data[SAFE_ACCESS(*payload_pos)],
                                                limit,
                                                (char *)charpointer);
    }

    if (written_bytes < 0) {
        return 0;
    }

	if (written_bytes == 0) {
		*((char *)&data[SAFE_ACCESS(*payload_pos)]) = '\0';
		written_bytes = 1;
	}

    *payload_pos += written_bytes;
    return (uint16_t)written_bytes;
}

static inline uint16_t linx_push_bytebuf(uint8_t *data,
                                            uint64_t *payload_pos,
                                            unsigned long bytebuf_pointer,
                                            uint16_t len_to_read,
                                            read_memory_t mem)
{
    if (mem == KERNEL) {
        if(bpf_probe_read_kernel(&data[SAFE_ACCESS(*payload_pos)],
                                 len_to_read,
                                 (void *)bytebuf_pointer) != 0)
        {
            return 0;
        }
    } else {
        if(bpf_probe_read_user(&data[SAFE_ACCESS(*payload_pos)],
                               len_to_read,
                               (void *)bytebuf_pointer) != 0)
        {
            return 0;
        }
    }

    *payload_pos += len_to_read;
    return len_to_read;
}

/**
 * 读取字符指针
 */
static inline uint16_t linx_ringbuf_store_charpointer(linx_ringbuf_t *ringbuf, 
                                                    unsigned long charpointer,
                                                    uint16_t len_to_read,
                                                    read_memory_t mem)
{
    uint16_t charbuf_len = 0;

	if (charpointer) {
        charbuf_len = linx_push_charpointer(ringbuf->data,
                                            &ringbuf->payload_pos,
                                            charpointer,
                                            len_to_read,
                                            mem);
    }

    ((linx_event_t *)ringbuf->data)->params_size[ringbuf->index++] = (uint64_t)charbuf_len;

    return charbuf_len;
}

/**
 * 读取字符数组
 */
static inline uint16_t linx_ringbuf_store_bytebuf(linx_ringbuf_t *ringbuf, 
                                                  unsigned long charpointer,
                                                  uint16_t len_to_read,
                                                  read_memory_t mem)
{
    uint16_t bytebuf_len = 0;

    if (charpointer && len_to_read > 0) {
        bytebuf_len = linx_push_bytebuf(ringbuf->data,
                                        &ringbuf->payload_pos,
                                        charpointer,
                                        len_to_read,
                                        mem);
    }

    ((linx_event_t *)ringbuf->data)->params_size[ringbuf->index++] = (uint64_t)bytebuf_len;

    return bytebuf_len;
}

static inline void linx_ringbuf_store_charbufarray_as_bytebuf(linx_ringbuf_t *ringbuf,
                                                              unsigned long start_pointer,
                                                              uint16_t len_to_read,
                                                              uint16_t max_len)
{
    if (len_to_read >= max_len) {
        len_to_read = max_len;
    }

    if (linx_ringbuf_store_bytebuf(ringbuf, start_pointer, len_to_read, USER) > 0) {
        linx_push_previous_character(ringbuf->data, &ringbuf->payload_pos, '\0');
    }
}

static inline void linx_ringbuf_store_socktuple(linx_ringbuf_t *ringbuf, 
                                                uint32_t socket_fd,
                                                int direction,
                                                struct sockaddr *usrsockaddr)
{
    uint16_t final_param_len = 0;
    uint16_t socket_family = 0;
    struct file *file = extract__file_struct_from_fd(socket_fd);
    struct socket *socket = get_sock_from_file(file);
    struct sock *sk;

    if (socket == NULL) {
        linx_ringbuf_store_empty(ringbuf);
        return;
    }

    sk = BPF_CORE_READ(socket, sk);
    if (sk == NULL) {
        linx_ringbuf_store_empty(ringbuf);
        return;
    }

    BPF_CORE_READ_INTO(&socket_family, sk, __sk_common.skc_family);

    switch (socket_family) {
    case AF_INET: {
        struct inet_sock *inet = (struct inet_sock *)sk;
        uint32_t ipv4_local = 0;
        uint16_t port_local = 0;
        uint32_t ipv4_remote = 0;
        uint16_t port_remote = 0;

        BPF_CORE_READ_INTO(&ipv4_local, inet, inet_saddr);
        BPF_CORE_READ_INTO(&port_local, inet, inet_sport);
        BPF_CORE_READ_INTO(&ipv4_remote, sk, __sk_common.skc_daddr);
        BPF_CORE_READ_INTO(&port_remote, sk, __sk_common.skc_dport);

        if (port_remote == 0 && usrsockaddr != NULL) {
            struct sockaddr_in *usrsockaddr_in = {};
            bpf_probe_read_user(&usrsockaddr_in, 
                                bpf_core_type_size(struct sockaddr_in),
                                (void *)usrsockaddr);
            ipv4_remote = usrsockaddr_in->sin_addr.s_addr;
            port_remote = usrsockaddr_in->sin_port;
        }

        PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, (uint8_t)socket_family, uint8_t);

        if (direction == OUTBOUND) {
            PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, (uint32_t)ipv4_local, uint32_t);
            PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, (uint16_t)ntohs(port_local), uint16_t);
            PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, (uint32_t)ipv4_remote, uint32_t);
            PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, (uint16_t)ntohs(port_remote), uint16_t);
        } else {
            PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, (uint32_t)ipv4_remote, uint32_t);
            PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, (uint16_t)ntohs(port_remote), uint16_t);
            PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, (uint32_t)ipv4_local, uint32_t);
            PUSH_FIXDE_VALUE_TO_RINGBUF(ringbuf, (uint16_t)ntohs(port_local), uint16_t);
        }

        final_param_len = LINX_FAMILY_SIZE + LINX_IPV4_SIZE * 2 + LINX_PORT_SIZE * 2;
        break;
    }

    case AF_INET6: {
        break;
    }

    case AF_UNIX: {
        break;
    }

    default:
        final_param_len = 0;
        break;
    }

    PUSH_FIXED_SIZE_TO_RINGBUF(ringbuf, final_param_len);
}

static inline long extract__msghdr(struct user_msghdr *msghdr,
                                   unsigned long msghdr_pointer) {
	return bpf_probe_read_user((void *)msghdr,
	                           bpf_core_type_size(struct user_msghdr),
	                           (void *)msghdr_pointer);
}

static inline void apply_snaplen(struct pt_regs *regs, uint16_t *snaplen, const snaplen_args_t *input_args)
{
    unsigned long args[5] = {0};
	struct sockaddr *sockaddr = NULL;
	union {
		struct compat_msghdr compat_mh;
		struct user_msghdr mh;
		struct compat_mmsghdr compat_mmh;
		struct mmsghdr mmh;
	} msg_mh = {};

    switch (input_args->type) {
	case LINX_EVENT_TYPE_SENDTO_X:
	case LINX_EVENT_TYPE_RECVFROM_X:
		extract__network_args(args, 5, regs);
		sockaddr = (struct sockaddr *)args[4];
		break;
    
    case LINX_EVENT_TYPE_RECVMSG_X:
    case LINX_EVENT_TYPE_SENDMSG_X: {
        extract__network_args(args, 3, regs);

        if(extract__msghdr(&msg_mh.mh, args[1]) == 0) {
			sockaddr = (struct sockaddr *)msg_mh.mh.msg_name;
		}
    } break;

	case LINX_EVENT_TYPE_RECVMMSG_X:
	case LINX_EVENT_TYPE_SENDMMSG_X: {
        __builtin_memcpy(args, input_args->mm_args, 3 * sizeof(unsigned long));

		struct mmsghdr *mmh_ptr = (struct mmsghdr *)args[1];
		if(bpf_probe_read_user(&msg_mh.mmh,
		                       bpf_core_type_size(struct mmsghdr),
		                       (void *)(mmh_ptr + input_args->mmsg_index)) == 0) {
			sockaddr = (struct sockaddr *)msg_mh.mmh.msg_hdr.msg_name;
		}
    } break;

    default:
        extract__network_args(args, 3, regs);
        break;
    }

    int32_t socket_fd = (int32_t)args[0];
    if (socket_fd < 0) {
        return;
    }

    struct file *file = extract__file_struct_from_fd(socket_fd);
    struct socket *socket = get_sock_from_file(file);
    if (socket == NULL) {
        return;
    }

	struct sock *sk = BPF_CORE_READ(socket, sk);
	if(sk == NULL) {
		return;
	}

    uint16_t port_local = 0;
    uint16_t port_remote = 0;

	uint16_t socket_family = BPF_CORE_READ(sk, __sk_common.skc_family);
	if(socket_family == AF_INET || socket_family == AF_INET6) {
		struct inet_sock *inet = (struct inet_sock *)sk;
		BPF_CORE_READ_INTO(&port_local, inet, inet_sport);
		BPF_CORE_READ_INTO(&port_remote, sk, __sk_common.skc_dport);
		port_local = ntohs(port_local);
		port_remote = ntohs(port_remote);

		if(port_remote == 0 && sockaddr != NULL) {
			union {
				struct sockaddr_in sockaddr_in;
				struct sockaddr_in6 sockaddr_in6;
			} saddr_in = {};
			if(socket_family == AF_INET) {
				bpf_probe_read_user(&saddr_in.sockaddr_in,
				                    bpf_core_type_size(struct sockaddr_in),
				                    sockaddr);
				port_remote = ntohs(saddr_in.sockaddr_in.sin_port);
			} else {
				bpf_probe_read_user(&saddr_in.sockaddr_in6,
				                    bpf_core_type_size(struct sockaddr_in6),
				                    sockaddr);
				port_remote = ntohs(saddr_in.sockaddr_in6.sin6_port);
			}
		}
	}

    return;
}

#endif /* __RINGBUF_FUNC_H__ */
