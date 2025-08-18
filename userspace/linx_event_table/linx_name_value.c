#include "linx_name_value.h"

const linx_name_value_t file_flags[] = {
    {"O_LARGEFILE", LINX_O_LARGEFILE},
    {"O_DIRECTORY", LINX_O_DIRECTORY},
    {"O_DIRECT", LINX_O_DIRECT},
    {"O_TRUNC", LINX_O_TRUNC},
    {"O_SYNC", LINX_O_SYNC},
    {"O_NONBLOCK", LINX_O_NONBLOCK},
    {"O_EXCL", LINX_O_EXCL},
    {"O_DSYNC", LINX_O_DSYNC},
    {"O_APPEND", LINX_O_APPEND},
    {"O_CREAT", LINX_O_CREAT},
    {"O_RDWR", LINX_O_RDWR},
    {"O_WRONLY", LINX_O_WRONLY},
    {"O_RDONLY", LINX_O_RDONLY},
    {"O_CLOEXEC", LINX_O_CLOEXEC},
    {"O_TMPFILE", LINX_O_TMPFILE},
    {0, 0}
};
