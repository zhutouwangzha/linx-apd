#ifndef __LINX_MACHINE_STATUS_H__
#define __LINX_MACHINE_STATUS_H__ 

#include "user_struct.h"
#include "group_struct.h"

int linx_machine_status_init(void);

void linx_machine_status_deinit(void);

user_t *linx_machine_status_get_user(void);

group_t *linx_machine_status_get_group(void);

#endif /* __LINX_MACHINE_STATUS_H__ */
