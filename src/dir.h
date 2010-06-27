#ifndef _dir_h
#define _dir_h

#include <stdlib.h>
#include <sys/sendfile.h>

enum {
    MAX_SEND_BUFFER = 16 * 1024
};

int Dir_find_file(const char *path, size_t path_len, size_t *out_size);
int Dir_stream_file(int file_fd, size_t flen, int sock_fd);

#define Dir_send sendfile

#endif
