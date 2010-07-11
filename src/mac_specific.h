#ifndef _MAC_SPECIFIC_H
#define _MAC_SPECIFIC_H
#include <sys/types.h>

int mac_sendfile(int out_fd, int in_fd, off_t *offset, size_t count);

#endif
