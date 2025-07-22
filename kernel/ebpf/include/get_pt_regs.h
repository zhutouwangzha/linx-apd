#ifndef __GET_PT_REGS_H__
#define __GET_PT_REGS_H__

#include "bpf_common.h"

static inline unsigned long get_pt_regs_argumnet(struct pt_regs *regs, int idx)
{
    unsigned long arg;
    switch (idx) {
    case 0:
        arg = PT_REGS_PARM1_CORE_SYSCALL(regs);
        break;
    case 1:
        arg = PT_REGS_PARM2_CORE_SYSCALL(regs);
        break;
    case 2:
        arg = PT_REGS_PARM3_CORE_SYSCALL(regs);
        break;
    case 3:
        arg = PT_REGS_PARM4_CORE_SYSCALL(regs);
        break;
    case 4:
        arg = PT_REGS_PARM5_CORE_SYSCALL(regs);
        break;
    case 5:
        arg = PT_REGS_PARM6_CORE_SYSCALL(regs);
        break;
    default:
        arg = 0;
        break;
    }

    return arg;
}

static inline long get_syscall_id(struct pt_regs *regs)
{
    return regs->orig_ax;
}

#endif /* __GET_PT_REGS_H__ */
