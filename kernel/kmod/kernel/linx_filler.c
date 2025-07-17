#include <linux/types.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <asm-generic/access_ok.h>
#include "../include/kernel/linx_fillers.h"

#define syscall_get_arguments_deprecated(_args, _start, _len, _out)       \
	do {                                                                  \
		memcpy(_out, &_args->args[_start], _len * sizeof(unsigned long)); \
	} while(0)

#define CHECK_RES(x)                 \
	if(unlikely(x != 0)) {           \
		return x;                    \
	}


long ppm_strncpy_from_user(char *to, const char __user *from, unsigned long n) {
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


int val_to_ring(struct event_filler_arguments *args,
                uint64_t val,
                uint32_t val_len,
                bool fromuser,
                uint8_t dyn_idx) 
{
    const struct sysmon_param_info *param_info;
    int len = -1;
    uint32_t max_arg_size = args->arg_data_size;

    if (args->curarg >= args->nargs) {
        return -1;
    }

    if (args->arg_data_size == 0)
        return -1;

    param_info = &g_event_info[args->event_type].params[args->curarg];

    switch (param_info->type)
    {
    case PT_CHARBUF:
        if(unlikely(val == 0)) {
			len = 0;
			break;
		}
        if (fromuser) {
            len = ppm_strncpy_from_user(args->buffer + args->arg_data_offset,
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
        
    case PT_UINT32:
        if(likely(max_arg_size >= sizeof(uint32_t))) {
			*(uint32_t *)(args->buffer + args->arg_data_offset) = (uint32_t)val;
			len = sizeof(uint32_t);
		} else {
			return -1;
		}

        break;
    
    default:
        break;
    }

    args->args_len[args->curarg] = len;
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
    /*TODO: 参数解析并写入缓冲区*/
    return 0;
}

int f_sys_params_e_1(struct event_filler_arguments *args)
{
    /*TODO: 参数解析并写入缓冲区*/
    return 0;
}

int f_sys_params_x_1(struct event_filler_arguments *args)
{
    /*TODO: 参数解析并写入缓冲区*/
    return 0;
}