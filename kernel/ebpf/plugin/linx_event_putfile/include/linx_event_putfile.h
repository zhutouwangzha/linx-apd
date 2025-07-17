#ifndef __LINX_EVENT_PUTFILE_H__
#define __LINX_EVENT_PUTFILE_H__

#include <stdint.h>

typedef struct {
    char *map;
    off_t write_offset;
    off_t map_offset;
    size_t map_size;
} linx_event_putfile_t;

void linx_event_putfile(uint8_t *data);

void linx_event_putfile_clean(void);

int linx_event_open_file(char *file, int *fd);

#endif /* __LINX_EVENT_PUTFILE_H__ */
