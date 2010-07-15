#ifndef _dir_h
#define _dir_h

#include <stdlib.h>

#ifdef __APPLE__
#include "mac_specific.h"
#else
#include <sys/sendfile.h>
#endif
#include <bstring.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <request.h>

enum {
    MAX_SEND_BUFFER = 16 * 1024,
    MAX_DIR_PATH = 256
};

typedef struct FileRecord {
    int fd;
    time_t loaded;
    bstring date;
    bstring last_mod;
    bstring content_type;
    bstring header;
    bstring full_path;
    bstring etag;
    struct stat sb;
} FileRecord;

typedef struct Dir {
    bstring prefix;
    bstring base;
    bstring normalized_base;
    bstring index_file;
    bstring default_ctype;
} Dir;

Dir *Dir_create(const char *base, const char *prefix, const char *index_file,
        const char *default_ctype);

void Dir_destroy(Dir *dir);

FileRecord *Dir_find_file(bstring path, bstring default_type);

int Dir_stream_file(FileRecord *file, int sock_fd);

int Dir_serve_file(Request *req, Dir *dir, bstring path, int fd);

void FileRecord_destroy(FileRecord *file);

#ifdef __APPLE__
#define Dir_send mac_sendfile
#else
#define Dir_send sendfile
#endif

#endif
