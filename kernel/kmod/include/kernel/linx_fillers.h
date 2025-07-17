#ifndef __LINX_FILLERS_H__
#define __LINX_FILLERS_H__

#include "sysmon_events.h"

int f_sys_open_e(struct event_filler_arguments *args);
int f_sys_open_x(struct event_filler_arguments *args);
int f_sys_params_e_1(struct event_filler_arguments *args);
int f_sys_params_x_1(struct event_filler_arguments *args);

#endif /*__LINX_FILLERS_H__*/