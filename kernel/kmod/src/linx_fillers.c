#include <linux/types.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <asm/syscall.h>
#include <asm-generic/syscall.h>
#include <linux/fcntl.h>
#include <asm-generic/access_ok.h>
#include <net/inet_sock.h>
#include <linux/fdtable.h>
#include <linux/tty.h>
#include <linux/audit.h>

#include "linx_fillers.h"
#include "linx_events.h"
#include "linx_event_table.h"
#include "linx_field_type.h"
#include "linx_field_type.h"


# define UINT32_MAX		(4294967295U)
#ifdef PAGE_SIZE
#define STR_STORAGE_SIZE PAGE_SIZE
#endif

#define syscall_get_arguments_deprecated(_args, _start, _len, _out)       \
    do {                                                                  \
		memcpy(_out, &_args->args[_start], _len * sizeof(unsigned long)); \
	} while(0)

#define CHECK_RES(x)                 \
	if(unlikely(x != 0)) {           \
		return x;                    \
	}


long linx_strncpy_from_user(char *to, const char __user *from, unsigned long n) {
	long string_length = 0;
	long res = -1;
	unsigned long bytes_to_read = 4;
	int j;

	pagefault_disable();

	while(n) {

		if(n < bytes_to_read)
			bytes_to_read = n;

		if(!access_ok(from, bytes_to_read)) {
			res = -1;
			goto strncpy_end;
		}

		if(__copy_from_user_inatomic(to, from, bytes_to_read)) {
			/*
			 * Page fault
			 */
			res = -1;
			goto strncpy_end;
		}

		n -= bytes_to_read;
		from += bytes_to_read;

		for(j = 0; j < bytes_to_read; ++j) {
			++string_length;

			/* Check if `*to` is the `\0`. */
			if(!*to) {
				res = string_length;
				goto strncpy_end;
			}

			++to;
		}
	}
	res = string_length;

strncpy_end:
	pagefault_enable();
	return res;
}


int push_empty_param(struct event_filler_arguments *args) 
{
	if(unlikely(args->curarg >= args->nargs)) {
		return -1;
	}

	args->curarg++;
    args->args_len[args->curarg] = 0;
	return 0;
}


static inline void get_dev_ino_overlay_from_fd(int64_t fd,
                                               uint32_t *dev,
                                               uint64_t *ino) {
	struct files_struct *files;
	struct fdtable *fdt;
	struct inode *inode;
	struct super_block *sb;
	struct file *file;

	if(fd < 0)
		return;

	files = current->files;
	if(unlikely(!files))
		return;

	spin_lock(&files->file_lock);
	fdt = files_fdtable(files);
	if(unlikely(fd > fdt->max_fds))
		goto out_unlock;

	file = fdt->fd[fd];
	if(unlikely(!file))
		goto out_unlock;

	inode = file_inode(file);
	if(unlikely(!inode))
		goto out_unlock;

	*ino = inode->i_ino;

	sb = inode->i_sb;
	if(unlikely(!sb))
		goto out_unlock;

	*dev = new_encode_dev(sb->s_dev);

out_unlock:
	spin_unlock(&files->file_lock);
	return;
}

static __always_inline uint32_t dup3_flags_to_scap(int flags) {
	uint32_t res = 0;
#ifdef O_CLOEXEC
	if(flags & O_CLOEXEC)
		res |= SYSMON_O_CLOEXEC;
#endif
	return res;
}

static uint32_t linx_get_tty(void) {
	struct signal_struct *sig;
	struct tty_struct *tty;
	struct tty_driver *driver;
	int major;
	int minor_start;
	int index;
	uint32_t tty_nr = 0;

	sig = current->signal;
	if(!sig)
		return 0;

	if(unlikely(copy_from_kernel_nofault(&tty, &sig->tty, sizeof(tty))))
		return 0;

	if(!tty)
		return 0;

	if(unlikely(copy_from_kernel_nofault(&index, &tty->index, sizeof(index))))
		return 0;

	if(unlikely(copy_from_kernel_nofault(&driver, &tty->driver, sizeof(driver))))
		return 0;

	if(!driver)
		return 0;

	if(unlikely(copy_from_kernel_nofault(&major, &driver->major, sizeof(major))))
		return 0;

	if(unlikely(copy_from_kernel_nofault(&minor_start, &driver->minor_start, sizeof(minor_start))))
		return 0;

	tty_nr = new_encode_dev(MKDEV(major, minor_start) + index);

	return tty_nr;
}


static unsigned long linx_get_mm_counter(struct mm_struct *mm, int member) {
	long val = 0;
	val = get_mm_counter(mm, member);
	return val;
}

static unsigned long linx_get_mm_swap(struct mm_struct *mm) {
	return linx_get_mm_counter(mm, MM_SWAPENTS);
}

static unsigned long linx_get_mm_rss(struct mm_struct *mm) {
	return get_mm_rss(mm);
}

static int accumulate_argv_or_env(const void __user *argv, char *str_storage) {
	int len = 0;
	int ret = 0;
	const char __user *p = NULL;

	for(;;) {
		if(argv == NULL)
			break;

		if(unlikely(linx_copy_from_user(&p, argv, sizeof(p)))) {
			/* We return what we read until now */
			break;
		}

		if(p == NULL)
			break;

		/* ppm_strncpy_from_user includes the trailing \0 */
		ret = linx_copy_from_user(&str_storage[len], p, STR_STORAGE_SIZE - len);
		if(ret < 0) {
			/* We ignore the failed read. We will try to read from the same position in
			 * the next iteration.
			 */
			ret = 0;
		}

		len += ret;
		if(len >= STR_STORAGE_SIZE) {
			len = STR_STORAGE_SIZE;
			break;
		}

		argv += sizeof(argv);
	}

	if(len > 0) {
		str_storage[len - 1] = '\0';
	} else {
		str_storage[0] = '\0';
	}
	return len;
}



int val_to_ring(struct event_filler_arguments *args,
                uint64_t val,
                uint32_t val_len,
                bool fromuser,
                uint8_t dyn_idx) 
{
    const linx_param_info_t *param_info;
    int len = -1;
    uint32_t max_arg_size = args->arg_data_size;

    if (args->curarg >= args->nargs) {
        return -1;
    }

    if (args->arg_data_size == 0)
        return -1;

    param_info = (linx_param_info_t *)&g_linx_event_table[args->event_type].params[args->curarg];
    
    switch (param_info->type)
    {
    case LINX_FIELD_TYPE_CHARBUF:
    {
        if(unlikely(val == 0)) {
			len = 0;
			break;
		}
        if (fromuser) {
            len = linx_strncpy_from_user(args->buffer + args->arg_data_offset,
			                            (const char __user *)(unsigned long)val,
			                            max_arg_size);

			if(unlikely(len < 0)) {
				len = 0;
				break;
			}
            *(char *)(args->buffer + args->arg_data_offset + max_arg_size - 1) = '\0';
        } else {
            len = (int)strscpy(args->buffer + args->arg_data_offset,
			                   (const char *)(unsigned long)val,
			                   max_arg_size);
            if(len == -E2BIG) {
				len = max_arg_size;
			} else {
				len++;
			}
        }
        break;
    }
    case LINX_FIELD_TYPE_UINT32:
        if(likely(max_arg_size >= sizeof(uint32_t))) {
			*(uint32_t *)(args->buffer + args->arg_data_offset) = (uint32_t)val;
			len = sizeof(uint32_t);
		} else {
			return -1;
		}
        break;
    case LINX_FIELD_TYPE_INT32:
        if(likely(max_arg_size >= sizeof(int32_t))) {
			*(int32_t *)(args->buffer + args->arg_data_offset) = (uint32_t)val;
			len = sizeof(int32_t);
		} else {
			return -1;
		}
        break;
    
    case LINX_FIELD_TYPE_INT64:
        if(likely(max_arg_size >= sizeof(int64_t))) {
			*(int64_t *)(args->buffer + args->arg_data_offset) = (int64_t)(long)val;
			len = sizeof(int64_t);
		} else {
			return -1;
		}
        break;

    case LINX_FIELD_TYPE_UINT64:
        if(likely(max_arg_size >= sizeof(int64_t))) {
			*(uint64_t *)(args->buffer + args->arg_data_offset) = (uint64_t)val;
			len = sizeof(uint64_t);
		} else {
			return -1;
		}
        break;
#if 0
    case PT_SOCKADDR:
	case PT_SOCKTUPLE:
    case PT_BYTEBUF:
		if(likely(val != 0)) {
			if(unlikely(val_len >= max_arg_size))
				return -1;

			if(fromuser) {
				len = (int)linx_copy_from_user(args->buffer + args->arg_data_offset,
				                              (const void __user *)(unsigned long)val,
				                              val_len);

				if(unlikely(len != 0)) {
					goto send_empty_sock_param;
				}

				len = val_len;
			} else {
				memcpy(args->buffer + args->arg_data_offset, (void *)(unsigned long)val, val_len);

				len = val_len;
			}
			/* If we arrive here we have something to send. */
			break;
		}

    send_empty_sock_param:
		len = 0;
		break;
#endif
    default:
        break;
    }

    args->args_len[args->curarg] = (uint64_t)len;
    args->curarg++;
	args->arg_data_offset += len;
	args->arg_data_size -= len;

    return 0;
}

int f_sys_open_e(struct event_filler_arguments *args)
{
    unsigned long val;
	unsigned long flags;
	unsigned long modes;
	int res;
   
    /**
     * name
     */
    syscall_get_arguments_deprecated(args, 1, 1, &val);
	res = val_to_ring(args, val, 0, true, 0);
	CHECK_RES(res);
    
    /*
	 * Flags
	 */
	syscall_get_arguments_deprecated(args, 2, 1, &flags);
	res = val_to_ring(args, flags, 0, false, 0);
	// CHECK_RES(res);

	/*
	 *  mode
	 */
	syscall_get_arguments_deprecated(args, 3, 1, &modes);
	res = val_to_ring(args, modes, 0, false, 0);
	CHECK_RES(res);

	return res;
}

int f_sys_open_x(struct event_filler_arguments *args)
{
    return 0;
}

int f_sys_params_e_1(struct event_filler_arguments *args)
{
    /*TODO: 参数解析并写入缓冲区*/
    return 0;
}

int f_sys_params_x_1(struct event_filler_arguments *args)
{
    /* TODO: 参数解析并写入缓冲区 */
    return 0;
}

int f_sys_empty(struct event_filler_arguments *args)
{
    return 0;
}





int	f_sys_autofill(struct event_filler_arguments *args)
{
    return 0;
}                          
int	f_sys_generic(struct event_filler_arguments *args)
{
    return 0;
}                   
                      
int	f_sys_getcwd_x(struct event_filler_arguments *args) 
{
    return 0;
}                     
int	f_sys_getdents_e(struct event_filler_arguments *args)    
{
    return 0;
}                
int	f_sys_getdents64_e(struct event_filler_arguments *args)   
{
    return 0;
}                
int	f_sys_single(struct event_filler_arguments *args) 
{
    return 0;
}                        
int	f_sys_single_x(struct event_filler_arguments *args) 
{
    return 0;
}                     
int	f_sys_fstat_e(struct event_filler_arguments *args) 
{
    return 0;
}                      
                       
                     
int	f_sys_read_e(struct event_filler_arguments *args) 
{
    unsigned long val = 0;
	int res = 0;
	int32_t fd = 0;

	/*
     fd 
    */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	/*
     size 
    */
	syscall_get_arguments_deprecated(args, 2, 1, &val);
	res = val_to_ring(args, val, 0, false, 0);
	CHECK_RES(res);

    return res;
}                       
int	f_sys_read_x(struct event_filler_arguments *args)        
{
	unsigned long val;
	int res;
	int64_t retval;

	syscall_get_arguments_deprecated(args, 0, 1, &val);
	args->fd = (int)val;

	/* 
     res 
    */
	retval = (int64_t)(long)syscall_get_return_value(current, args->regs);
	res = val_to_ring(args, retval, 0, false, 0);
	CHECK_RES(res);

	/*
     data 
    */
	if(retval > 0) {
		syscall_get_arguments_deprecated(args, 1, 1, &val);
		if (retval > MAX_CMDLINE_LEN)
            retval = MAX_CMDLINE_LEN;
		res = val_to_ring(args, val, retval, true, 0);
	} else {
		res = push_empty_param(args);
	}
	CHECK_RES(res);

	/* 
     fd 
    */
	res = val_to_ring(args, (int64_t)args->fd, 0, false, 0);
	CHECK_RES(res);

	/*
     size  
    */
	syscall_get_arguments_deprecated(args, 2, 1, &val);
	res = val_to_ring(args, val, 0, false, 0);
	CHECK_RES(res);

	return res;
}                
int	f_sys_write_e(struct event_filler_arguments *args)
{
	unsigned long val = 0;
	int res = 0;
	int32_t fd = 0;

	/*
     fd  
    */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);

	CHECK_RES(res);

	/*
     size 
    */
	syscall_get_arguments_deprecated(args, 2, 1, &val);

	res = val_to_ring(args, val, 0, false, 0);
	CHECK_RES(res);


    return res;
}                       
int	f_sys_write_x(struct event_filler_arguments *args)
{
	unsigned long val;
	int res;
	int64_t retval;
	unsigned long bufsize;
	size_t size = 0;

	/*
	 * fd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	args->fd = (int)val;

	/*
     res 
    */
	retval = (int64_t)(long)syscall_get_return_value(current, args->regs);
	res = val_to_ring(args, retval, 0, false, 0);
	CHECK_RES(res);

	/*
     data 
    */
	/* Get the size from userspace paramater */
	syscall_get_arguments_deprecated(args, 2, 1, &size);
	bufsize = retval > 0 ? retval : size;
    if (bufsize > MAX_CMDLINE_LEN)
            bufsize = MAX_CMDLINE_LEN;
    
	syscall_get_arguments_deprecated(args, 1, 1, &val);
	res = val_to_ring(args, val, bufsize, true, 0);
	CHECK_RES(res);

	/*
     fd 
    */
	res = val_to_ring(args, (int64_t)args->fd, 0, false, 0);
	CHECK_RES(res);

	/*
     size 
    */
	res = val_to_ring(args, (uint32_t)size, 0, false, 0);
	CHECK_RES(res);

    return res;
}                       
int	f_sys_execve_e(struct event_filler_arguments *args)
{
    int res;
	unsigned long val;

	/*
	 * filename
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	res = val_to_ring(args, val, 0, true, 0);
	CHECK_RES(res);

	return res;
    return 0;
}                      
int	f_proc_startupdate(struct event_filler_arguments *args)
{
    int res;
	unsigned long val;
    int args_len = 0;
    int64_t retval;
    struct mm_struct *mm = current->mm;
    unsigned int exe_len = 0; 
    uint32_t tty_nr = 0;
    long total_vm = 0;
	long total_rss = 0;
	long swap = 0;
    long env_len = 0;
    uint32_t loginuid = UINT32_MAX;
    char *buf = args->str_storage;

    retval = (int64_t)syscall_get_return_value(current, args->regs);

    if(unlikely(retval < 0 && args->event_type != LINX_EVENT_TYPE_EXECVE_X)) {
        return 0;
    } else {
        if (likely(retval >= 0)) {

            if(unlikely(!mm)) {
				return -1;
			}

			if(unlikely(!mm->arg_end)) {
				printk("f_proc_startupdate drop, mm->arg_end=NULL\n");
				return -1;
			}

            args_len = mm->arg_end - mm->arg_start;
            if(args_len) {
				if(args_len > STR_STORAGE_SIZE)
					args_len = STR_STORAGE_SIZE;
                
                if (unlikely(linx_copy_from_user(buf, (const void __user*)mm->arg_start, args_len)))
                    args_len = 0;
                else
                    buf[args_len - 1] = 0;
            }
        } else {
            syscall_get_arguments_deprecated(args, 1, 1, &val);
        }

        if (args_len == 0) 
            *buf = 0;
        
        exe_len = strnlen(buf, args_len);
        if(exe_len < args_len)
			++exe_len;


        /*
		 * exe
		 */
		res = val_to_ring(args, (uint64_t)(long)buf, exe_len, false, 0);
		CHECK_RES(res);

    }

    if(mm) {
		total_vm = mm->total_vm << (PAGE_SHIFT - 10);
		total_rss = linx_get_mm_rss(mm) << (PAGE_SHIFT - 10);
		swap = linx_get_mm_swap(mm) << (PAGE_SHIFT - 10);
	}

    /*
	 * vm_size
	 */
	res = val_to_ring(args, total_vm, 0, false, 0);
	CHECK_RES(res);

	/*
	 * vm_rss
	 */
	res = val_to_ring(args, total_rss, 0, false, 0);
	CHECK_RES(res);

	/*
	 * comm
	 */
	res = val_to_ring(args, (uint64_t)current->comm, 0, false, 0);
	CHECK_RES(res);

    /*
    * tty
    */
    tty_nr = linx_get_tty();
    res = val_to_ring(args, tty_nr, 0, false, 0);
    CHECK_RES(res);

    /*
    * env
    */
    if(likely(retval >= 0)) {
        env_len = mm->env_end - mm->env_start;
        memset(buf, 0, sizeof(buf));
        if(env_len) {
				if(env_len > STR_STORAGE_SIZE)
					env_len = STR_STORAGE_SIZE;

				if(unlikely(linx_copy_from_user(buf,
				                               (const void __user *)mm->env_start,
				                               env_len)))
					env_len = 0;
				else
					buf[env_len - 1] = 0;
		}
        for (size_t i = 0; i < env_len; i++)
        {
            if (buf[i] == '\0')
                buf[i] = ' ';
        }
        
    } else {
        syscall_get_arguments_deprecated(args, 2, 1, &val);
        env_len = accumulate_argv_or_env((const char __user *)val, buf);
    }

    if(env_len == 0)
    	buf[0] = 0;

    res = val_to_ring(args, (int64_t)(long)buf, env_len, false, 0);
    CHECK_RES(res);


    /*
     * loginuid
    */
    loginuid = from_kuid(current_user_ns(), audit_get_loginuid(current));
    res = val_to_ring(args, loginuid, 0, false, 0);
	CHECK_RES(res);

    /*
     * pgid
    */
    int64_t tmp_pgid = (int64_t)task_pgrp_nr_ns(current, task_active_pid_ns(current));

    res = val_to_ring(args,
		                  (int64_t)tmp_pgid,
		                  0,
		                  false,
		                  0);
    CHECK_RES(res);

	return res;
}   
                
int	f_proc_startupdate_2(struct event_filler_arguments *args)
{
    return 0;
}               
int	f_proc_startupdate_3(struct event_filler_arguments *args) 
{
    return 0;
}               
int	f_sys_socketpair_x(struct event_filler_arguments *args)
{
    return 0;
}                  
int	f_sys_setsockopt_x(struct event_filler_arguments *args)
{
    return 0;
}  

int	f_sys_getsockopt_x(struct event_filler_arguments *args)
{
    return 0;
}                  
int	f_sys_connect_x(struct event_filler_arguments *args) 
{
    return 0;
}                    
int	f_sys_accept4_e(struct event_filler_arguments *args)
{
    return 0;
}                     
int	f_sys_accept_x(struct event_filler_arguments *args) 
{
    return 0;
}                     

static int f_sys_send_e_common(struct event_filler_arguments *args, int *fd) {
	int res;
	unsigned long size;
	unsigned long val;
	int32_t tmp_fd = 0;

	/*
	 * fd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	tmp_fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)tmp_fd, 0, false, 0);
	CHECK_RES(res);

	*fd = val;

	/*
	 * size
	 */
	syscall_get_arguments_deprecated(args, 2, 1, &size);

	res = val_to_ring(args, size, 0, false, 0);
	CHECK_RES(res);

	return 0;
}


int	f_sys_send_e(struct event_filler_arguments *args)
{
    return 0;
}                        
int	f_sys_send_x(struct event_filler_arguments *args)
{
    unsigned long val;
	int res;
	int64_t retval;
	unsigned long bufsize;

	/*
	 * fd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);

	args->fd = (int)val;

	/* 
    * res 
    */
	retval = (int64_t)(long)syscall_get_return_value(current, args->regs);
	res = val_to_ring(args, retval, 0, false, 0);
	CHECK_RES(res);

	/* If the syscall doesn't fail we use the return value as `size`
	 * 
	 */
	syscall_get_arguments_deprecated(args, 2, 1, &val);
	bufsize = retval > 0 ? retval : val;
    
    if (bufsize > MAX_CMDLINE_LEN) {
        bufsize = MAX_CMDLINE_LEN;
    }
	
    syscall_get_arguments_deprecated(args, 1, 1, &val);

	
	res = val_to_ring(args, val, bufsize, true, 0);
	CHECK_RES(res);
    
    return res;
}                        
int	f_sys_sendto_e(struct event_filler_arguments *args)
{
	unsigned long val;
	int res;
	uint16_t size = 0;
    char arg_str[512] = {0};
	char *targetbuf = arg_str;
	int fd;
	struct sockaddr __user *usrsockaddr;
	struct sockaddr_storage address;
	int err = 0;

	*targetbuf = 250;

	/*
	 * Push the common params to the ring
	 */
	res = f_sys_send_e_common(args, &fd);
	CHECK_RES(res);

	/*
	 * Get the address
	 */
	syscall_get_arguments_deprecated(args, 4, 1, &val);
	usrsockaddr = (struct sockaddr __user *)val;

	/*
	 * Get the address len
	 */
	syscall_get_arguments_deprecated(args, 5, 1, &val);

	if(usrsockaddr != NULL && val != 0) {
		/*
		 * Copy the address
		 */
		err = addr_to_kernel(usrsockaddr, val, (struct sockaddr *)&address);
		if(likely(err >= 0)) {
			/*
             * 转换节点信息
			 */
			size = fd_to_socktuple(fd,
			                       (struct sockaddr *)&address,
			                       val,
			                       true,
			                       false,
			                       targetbuf,
			                       sizeof(arg_str));
		}
	}

	/*
	 * 拷贝到缓存区
	 */
	res = val_to_ring(args, (uint64_t)(unsigned long)arg_str, size, false, 0);
	CHECK_RES(res);

	return res;
}                      
int	f_sys_sendmsg_e(struct event_filler_arguments *args)
{
    return 0;
}                     
int	f_sys_sendmsg_x(struct event_filler_arguments *args)
{
    return 0;
}                     
int	f_sys_sendmmsg_x(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_sendmmsg_x_failure(struct event_filler_arguments *args) 
{
    return 0;
}           

static int f_sys_recv_x_common(struct event_filler_arguments *args, int64_t *retval) {
	int res;
	unsigned long val;
	unsigned long bufsize;

	/*
	 * fd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);

	args->fd = (int)val;

	/*
	 * res
	 */
	*retval = (int64_t)(long)syscall_get_return_value(current, args->regs);
	res = val_to_ring(args, *retval, 0, false, 0);
	CHECK_RES(res);

	/*
	 * data
	 */
	if(*retval < 0) {
		val = 0;
		bufsize = 0;
	} else {
		syscall_get_arguments_deprecated(args, 1, 1, &val);
		bufsize = *retval;
	}

    if (bufsize > MAX_CMDLINE_LEN) {
        bufsize = MAX_CMDLINE_LEN;
    }

	res = val_to_ring(args, val, bufsize, true, 0);
    
	return res;
}



int	f_sys_recv_x(struct event_filler_arguments *args)
{
    return 0;
}                        
int	f_sys_recvfrom_x(struct event_filler_arguments *args)
{
	unsigned long val;
	int res;
	uint16_t size = 0;
	int64_t retval;
    char str_buffer[512] = {0};
	char *targetbuf = str_buffer;
	int fd;
	struct sockaddr __user *usrsockaddr;
	struct sockaddr_storage address;
	int addrlen;
	int err = 0;

	/*
	 * Push the common params to the ring
	 */
	res = f_sys_recv_x_common(args, &retval);
	CHECK_RES(res);

	if(retval >= 0) {
		/*
		 * Get the fd
		 */
		syscall_get_arguments_deprecated(args, 0, 1, &val);
		fd = (int)val;

		/*
		 * Get the address
		 */
		syscall_get_arguments_deprecated(args, 4, 1, &val);
		usrsockaddr = (struct sockaddr __user *)val;

		/*
		 * Get the address len
		 */
		syscall_get_arguments_deprecated(args, 5, 1, &val);
		if(usrsockaddr != NULL && val != 0) {
			if(unlikely(linx_copy_from_user(&addrlen, (const void __user *)val, sizeof(addrlen))))
				return -2;
			/*
			 * 获取地址
			 */
			err = addr_to_kernel(usrsockaddr, addrlen, (struct sockaddr *)&address);
			if(likely(err >= 0)) {
				/*
				 * 转换为套接字信息
				 */
				size = fd_to_socktuple(fd,
				                       (struct sockaddr *)&address,
				                       addrlen,
				                       true,
				                       true,
				                       targetbuf,
				                       sizeof(str_buffer));
			}
		} else {
			/*
			 * 从fd中获取端点信息
			 */
			size = fd_to_socktuple(fd, NULL, 0, false, true, targetbuf, sizeof(str_buffer));
		}
	}

	/*
	 * 写入套接字端点信息
	 */
	res = val_to_ring(args, (uint64_t)(unsigned long)targetbuf, size, false, 0);
	CHECK_RES(res);

	return res;
}                    
int	f_sys_recvmsg_x(struct event_filler_arguments *args)
{
    return 0;
}                     
int	f_sys_recvmsg_x_2(struct event_filler_arguments *args) 
{
    return 0;
}                  
int	f_sys_recvmmsg_x(struct event_filler_arguments *args) 
{
    return 0;
}                   
int	f_sys_recvmmsg_x_2(struct event_filler_arguments *args) 
{
    return 0;
}                 
int	f_sys_shutdown_e(struct event_filler_arguments *args) 
{
    return 0;
}                   
int	f_sys_creat_e(struct event_filler_arguments *args)  
{
    return 0;
}                     
int	f_sys_creat_x(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_pipe_x(struct event_filler_arguments *args)  
{
    return 0;
}                      
int	f_sys_eventfd_e(struct event_filler_arguments *args) 
{
    return 0;
}                    
int	f_sys_futex_e(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_lseek_e(struct event_filler_arguments *args)  
{
    return 0;
}                     
int	f_sys_llseek_e(struct event_filler_arguments *args)
{
    return 0;
}                      
int	f_sys_socket_bind_x(struct event_filler_arguments *args)
{
    return 0;
}                 
int	f_sys_poll_e(struct event_filler_arguments *args)   
{
    return 0;
}                     
int	f_sys_poll_x(struct event_filler_arguments *args)  
{
    return 0;
}                      
int	f_sys_pread64_e(struct event_filler_arguments *args) 
{
    return 0;
}                    
int	f_sys_writev_e(struct event_filler_arguments *args)  
{
    return 0;
}                    
int	f_sys_pwrite64_e(struct event_filler_arguments *args) 
{
    return 0;
}                   
int	f_sys_readv_e(struct event_filler_arguments *args)
{
    return 0;
}                       
int	f_sys_preadv_e(struct event_filler_arguments *args) 
{
    return 0;
}                     
int	f_sys_readv_preadv_x(struct event_filler_arguments *args) 
{
    return 0;
}               
int	f_sys_writev_pwritev_x(struct event_filler_arguments *args) 
{
    return 0;
}             
int	f_sys_pwritev_e(struct event_filler_arguments *args) 
{
    return 0;
}                    
int	f_sys_nanosleep_e(struct event_filler_arguments *args) 
{
    return 0;
}                  
int	f_sys_getrlimit_setrlimit_e(struct event_filler_arguments *args)  
{
    return 0;
}       
int	f_sys_getrlimit_x(struct event_filler_arguments *args) 
{
    return 0;
}                  
int	f_sys_setrlimit_x(struct event_filler_arguments *args) 
{
    return 0;
}                  
int	f_sys_prlimit_e(struct event_filler_arguments *args)
{
    return 0;
}                     
int	f_sys_prlimit_x(struct event_filler_arguments *args) 
{
    return 0;
}                    
int	f_sched_switch_e(struct event_filler_arguments *args) 
{
    return 0;
}                   
int	f_sched_drop(struct event_filler_arguments *args)
{
    return 0;
}                        
int	f_sys_fcntl_e(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_fcntl_x(struct event_filler_arguments *args)
{
    return 0;
}                       
int	f_sys_ptrace_e(struct event_filler_arguments *args) 
{
    return 0;
}                     
int	f_sys_ptrace_x(struct event_filler_arguments *args) 
{
    return 0;
}                     
int	f_sys_mmap_e(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_brk_munmap_mmap_x(struct event_filler_arguments *args)  
{
    return 0;
}           
int	f_sys_renameat_x(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_renameat2_x(struct event_filler_arguments *args)
{
    return 0;
}                   
int	f_sys_symlinkat_x(struct event_filler_arguments *args)
{
    return 0;
}                   
int	f_sys_procexit_e(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_sendfile_e(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_sendfile_x(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_quotactl_e(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_quotactl_x(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_scapevent_e(struct event_filler_arguments *args)
{
    return 0;
}                   
int	f_sys_getresuid_and_gid_x(struct event_filler_arguments *args) 
{
    return 0;
}          
int	f_sys_signaldeliver_e(struct event_filler_arguments *args)
{
    return 0;
}               
int	f_sys_pagefault_e(struct event_filler_arguments *args)
{
    return 0;
}                   
int	f_sys_setns_e(struct event_filler_arguments *args)
{
    return 0;
}                       
int	f_sys_unshare_e(struct event_filler_arguments *args)  
{
    return 0;
}                   
int	f_sys_flock_e(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_cpu_hotplug_e(struct event_filler_arguments *args) 
{
    return 0;
}                    
int	f_sys_semop_x(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_semget_e(struct event_filler_arguments *args) 
{
    return 0;
}                     
int	f_sys_semctl_e(struct event_filler_arguments *args) 
{
    return 0;
}                     
int	f_sys_ppoll_e(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_mount_e(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_access_e(struct event_filler_arguments *args)
{
    return 0;
}                      
int	f_sys_socket_x(struct event_filler_arguments *args) 
{
    return 0;
}                     
int	f_sys_bpf_x(struct event_filler_arguments *args)
{
    return 0;
}                         
int	f_sys_unlinkat_x(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_fchmodat_x(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_chmod_x(struct event_filler_arguments *args)
{
    return 0;
}                       
int	f_sys_fchmod_x(struct event_filler_arguments *args)
{
    return 0;
}                      
int	f_sys_chown_x(struct event_filler_arguments *args)
{
    return 0;
}                       
int	f_sys_lchown_x(struct event_filler_arguments *args)
{
    return 0;
}                      
int	f_sys_fchown_x(struct event_filler_arguments *args)
{
    return 0;
}                      
int	f_sys_fchownat_x(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_mkdirat_x(struct event_filler_arguments *args)
{
    return 0;
}                     
int	f_sys_openat_e(struct event_filler_arguments *args)
{
    unsigned long val;
	unsigned long flags;
	unsigned long modes;
    int32_t fd;
	int res;
   
    /*
	 * dirfd
	 */
    syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	if(fd == AT_FDCWD)
		fd = -100;

	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

    /**
     * name
     */
    syscall_get_arguments_deprecated(args, 1, 1, &val);
	res = val_to_ring(args, val, 0, true, 0);
	CHECK_RES(res);
    
    /*
	 * Flags
	 */
	syscall_get_arguments_deprecated(args, 2, 1, &flags);
	res = val_to_ring(args, flags, 0, false, 0);
	CHECK_RES(res);

	/*
	 *  mode
	 */
	syscall_get_arguments_deprecated(args, 3, 1, &modes);
	res = val_to_ring(args, modes, 0, false, 0);
	CHECK_RES(res);

	return res;
}                      
int	f_sys_openat_x(struct event_filler_arguments *args)
{
    unsigned long val;
	unsigned long flags;
	unsigned long modes;

	uint32_t dev = 0;
	uint64_t ino = 0;
	int res;
	int32_t fd;
	int64_t retval;

	retval = (int64_t)syscall_get_return_value(current, args->regs);
	res = val_to_ring(args, retval, 0, false, 0);
	CHECK_RES(res);

	/*
	 * dirfd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	if(fd == AT_FDCWD)
		fd = -100;

	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	/*
	 * name
	 */
	syscall_get_arguments_deprecated(args, 1, 1, &val);
	res = val_to_ring(args, val, 0, true, 0);
	CHECK_RES(res);

	get_dev_ino_overlay_from_fd(retval, &dev, &ino);
	/*
	 * Flags
	 * Note that we convert them into the ppm portable representation before pushing them to the
	 * ring
	 */
	syscall_get_arguments_deprecated(args, 2, 1, &flags);
	res = val_to_ring(args, flags, 0, false, 0);
	CHECK_RES(res);
	/*
	 *  mode
	 */
	syscall_get_arguments_deprecated(args, 3, 1, &modes);
	res = val_to_ring(args, modes, 0, false, 0);
	CHECK_RES(res);

	/*
	 *  dev
	 */
	res = val_to_ring(args, dev, 0, false, 0);
	CHECK_RES(res);
	/*
	 *  ino
	 */
	res = val_to_ring(args, ino, 0, false, 0);
	CHECK_RES(res);

	return res;
}                      
int	f_sys_openat2_e(struct event_filler_arguments *args)
{
    return 0;
}                     
int	f_sys_openat2_x(struct event_filler_arguments *args)
{
    return 0;
}                     
int	f_sys_linkat_x(struct event_filler_arguments *args)
{
    return 0;
}                      
int	f_sys_mprotect_e(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_mprotect_x(struct event_filler_arguments *args)
{
    return 0;
}                    
int	f_sys_execveat_e(struct event_filler_arguments *args) 
{
    int res;
	unsigned long val;
	unsigned long flags;
	int32_t fd;

	/*
	 * dirfd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	if(fd == AT_FDCWD) {
		fd = -100;
	}

	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	/*
	 * pathname
	 */
	syscall_get_arguments_deprecated(args, 1, 1, &val);
	res = val_to_ring(args, val, 0, true, 0);
	CHECK_RES(res);

	/*
	 * flags
	 */
    syscall_get_arguments_deprecated(args, 4, 1, &flags);
	// syscall_get_arguments_deprecated(args, 4, 1, &val);
	// flags = execveat_flags_to_scap(val);

	res = val_to_ring(args, flags, 0, false, 0);
	CHECK_RES(res);

    return res;
}                   
int	f_execve_extra_tail_1(struct event_filler_arguments *args) 
{
    return 0;
}              
int	f_execve_extra_tail_2(struct event_filler_arguments *args)   
{
    return 0;
}            
int	f_sys_copy_file_range_e(struct event_filler_arguments *args)
{
    return 0;
}             
int	f_sys_copy_file_range_x(struct event_filler_arguments *args)  
{
    return 0;
}           
int	f_sys_connect_e(struct event_filler_arguments *args) 
{
    return 0;
}                    
int	f_sys_open_by_handle_at_x(struct event_filler_arguments *args) 
{
    return 0;
}          
int	open_by_handle_at_x_extra_tail_1(struct event_filler_arguments *args) 
{
    return 0;
}  
int	f_sys_io_uring_setup_x(struct event_filler_arguments *args) 
{
    return 0;
}             
int	f_sys_io_uring_enter_x(struct event_filler_arguments *args) 
{
    return 0;
}             
int	f_sys_io_uring_register_x(struct event_filler_arguments *args) 
{
    return 0;
}          
int	f_sys_mlock_x(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_munlock_x(struct event_filler_arguments *args) 
{
    return 0;
}                    
int	f_sys_mlockall_x(struct event_filler_arguments *args) 
{
    return 0;
}                   
int	f_sys_munlockall_x(struct event_filler_arguments *args)  
{
    return 0;
}                
int	f_sys_capset_x(struct event_filler_arguments *args) 
{
    return 0;
}                     
int	f_sys_dup2_e(struct event_filler_arguments *args)  
{
	int res;
	unsigned long val;
	int32_t fd = 0;

	/*
	 * oldfd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	return res;
}                      
int	f_sys_dup2_x(struct event_filler_arguments *args) 
{
    int res;
	unsigned long val;
	int32_t fd = 0;

	int64_t retval = (int64_t)syscall_get_return_value(current, args->regs);
	res = val_to_ring(args, retval, 0, false, 0);
	CHECK_RES(res);

	/*
	 * oldfd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	/*
	 * newfd
	 */
	syscall_get_arguments_deprecated(args, 1, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	return res;
}                       
int	f_sys_dup3_e(struct event_filler_arguments *args)  
{
    int res;
	unsigned long val;
	int32_t fd = 0;

	/*
	 * oldfd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	return res;
}                      
int	f_sys_dup3_x(struct event_filler_arguments *args)  
{
    int res;
	unsigned long val;
	int32_t fd = 0;

	int64_t retval = (int64_t)syscall_get_return_value(current, args->regs);
	res = val_to_ring(args, retval, 0, false, 0);
	CHECK_RES(res);

	/*
	 * oldfd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	/*
	 * newfd
	 */
	syscall_get_arguments_deprecated(args, 1, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	/*
	 * flags
	 */
	syscall_get_arguments_deprecated(args, 2, 1, &val);
	res = val_to_ring(args, dup3_flags_to_scap((int)val), 0, false, 0);
	CHECK_RES(res);

	return res;
}                      
int	f_sys_dup_e(struct event_filler_arguments *args) 
{
    int res;
	unsigned long val;
	int32_t fd = 0;

	/*
	 * oldfd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	return res;
}                        
int	f_sys_dup_x(struct event_filler_arguments *args)  
{
	int res;
	unsigned long val;
	int32_t fd = 0;

	int64_t retval = (int64_t)syscall_get_return_value(current, args->regs);
	res = val_to_ring(args, retval, 0, false, 0);
	CHECK_RES(res);

	/*
	 * oldfd
	 */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	return res;
}                       
int	f_sched_prog_exec(struct event_filler_arguments *args)  
{
    return 0;
}                 
int	f_sched_prog_exec_2(struct event_filler_arguments *args) 
{
    return 0;
}                
int	f_sched_prog_exec_3(struct event_filler_arguments *args) 
{
    return 0;
}                
int	f_sched_prog_exec_4(struct event_filler_arguments *args) 
{
    return 0;
}                
int	f_sched_prog_exec_5(struct event_filler_arguments *args) 
{
    return 0;
}                
int	f_sched_prog_fork(struct event_filler_arguments *args) 
{
    return 0;
}                  
int	f_sched_prog_fork_2(struct event_filler_arguments *args) 
{
    return 0;
}                
int	f_sched_prog_fork_3(struct event_filler_arguments *args) 
{
    return 0;
}                
int	f_sys_mlock2_x(struct event_filler_arguments *args) 
{
    return 0;
}                     
int	f_sys_fsconfig_x(struct event_filler_arguments *args)  
{
    return 0;
}                  
int	f_sys_epoll_create_e(struct event_filler_arguments *args)  
{
    return 0;
}              
int	f_sys_epoll_create_x(struct event_filler_arguments *args) 
{
    return 0;
}               
int	f_sys_epoll_create1_e(struct event_filler_arguments *args) 
{
    return 0;
}              
int	f_sys_epoll_create1_x(struct event_filler_arguments *args) 
{
    return 0;
}              
int	f_sys_socket_bind_e(struct event_filler_arguments *args) 
{
    return 0;
}                
int	f_sys_bpf_e(struct event_filler_arguments *args)  
{
    return 0;
}                       
int	f_sys_close_e(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_close_x(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_fchdir_e(struct event_filler_arguments *args)  
{
    return 0;
}                    
int	f_sys_fchdir_x(struct event_filler_arguments *args)  
{
    return 0;
}                    
int	f_sys_ioctl_e(struct event_filler_arguments *args)  
{
    return 0;
}                     
int	f_sys_mkdir_e(struct event_filler_arguments *args) 
{
    return 0;
}                      
int	f_sys_setpgid_e(struct event_filler_arguments *args)  
{
    return 0;
}                   
int	f_sys_recvfrom_e(struct event_filler_arguments *args) 
{
	int res = 0;
	unsigned long val = 0;
	int32_t fd = 0;

	/*
     fd  
    */
	syscall_get_arguments_deprecated(args, 0, 1, &val);
	fd = (int32_t)val;
	res = val_to_ring(args, (int64_t)fd, 0, false, 0);
	CHECK_RES(res);

	/*
     size 
    */
	syscall_get_arguments_deprecated(args, 2, 1, &val);
	res = val_to_ring(args, val, 0, false, 0);
	CHECK_RES(res);

	return res;
}                   
int	f_sys_recvmsg_e(struct event_filler_arguments *args) 
{
    return 0;
}

int	f_sys_listen_e(struct event_filler_arguments *args) 
{
    return 0;
}  

int	f_sys_listen_x(struct event_filler_arguments *args)  
{
    return 0;
}  

int	f_sys_signalfd_e(struct event_filler_arguments *args)  
{
    return 0;
} 

int	f_sys_splice_e(struct event_filler_arguments *args) 
{
    return 0;
}  

int	f_sys_umount_x(struct event_filler_arguments *args)  
{
    return 0;
} 

int	f_sys_umount2_e(struct event_filler_arguments *args)   
{
    return 0;
} 

int	f_sys_umount2_x(struct event_filler_arguments *args) 
{
    return 0;
} 

int	f_sys_pipe2_x(struct event_filler_arguments *args)  
{
    return 0;
} 

int	f_sys_inotify_init_e(struct event_filler_arguments *args)   
{
    return 0;
} 

int	f_sys_inotify_init1_x(struct event_filler_arguments *args)   
{
    return 0;
} 

int	f_sys_eventfd2_e(struct event_filler_arguments *args)   
{
    return 0;
} 

int	f_sys_eventfd2_x(struct event_filler_arguments *args)  
{
    return 0;
} 

int	f_sys_signalfd4_e(struct event_filler_arguments *args)    
{
    return 0;
} 

int	f_sys_signalfd4_x(struct event_filler_arguments *args)   
{
    return 0;
} 

int	f_sys_prctl_x(struct event_filler_arguments *args)    
{
    return 0;
}

int	f_sys_memfd_create_x(struct event_filler_arguments *args)    
{
    return 0;
} 

int	f_sys_pidfd_getfd_x(struct event_filler_arguments *args)   
{
    return 0;
} 

int	f_sys_pidfd_open_x(struct event_filler_arguments *args)    
{
    return 0;
}              
int	f_sys_init_module_x(struct event_filler_arguments *args)    
{
    return 0;
} 

int	f_sys_finit_module_x(struct event_filler_arguments *args)   
{
    return 0;
}

int	f_sys_mknod_x(struct event_filler_arguments *args)    
{
    return 0;
}   

int	f_sys_mknodat_x(struct event_filler_arguments *args)   
{
    return 0;
}  

int	f_sys_newfstatat_x(struct event_filler_arguments *args)  
{
    return 0;
}   

int	f_sys_process_vm_readv_x(struct event_filler_arguments *args)   
{
    return 0;
}  

int	f_sys_process_vm_writev_x(struct event_filler_arguments *args)   
{
    return 0;
} 

int	f_sys_delete_module_x(struct event_filler_arguments *args)    
{
    return 0;
}  

int	f_sys_pread64_x(struct event_filler_arguments *args)     
{
    return 0;
}  

int	f_sys_pwrite64_x(struct event_filler_arguments *args)   
{
    return 0;
}

int	f_terminate_filler(struct event_filler_arguments *args) 
{
    return 0;
}  