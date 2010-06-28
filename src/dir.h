#ifndef _dir_h
#define _dir_h

#include <stdlib.h>
#include <sys/sendfile.h>

enum {
    MAX_SEND_BUFFER = 16 * 1024,
    MAX_DIR_PATH = 256
};


typedef struct Dir {
    size_t base_len;
    char base[MAX_DIR_PATH];
} Dir;

Dir *Dir_create(const char *base);
void Dir_destroy(Dir *dir);

int Dir_find_file(const char *path, size_t path_len, size_t *out_size);

int Dir_stream_file(int file_fd, size_t flen, int sock_fd);

int Dir_serve_file(Dir *dir, const char *path, int fd);

#define Dir_send sendfile

#endif
