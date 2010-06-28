#include <dir.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dbg.h>
#include <task/task.h>
#include <string.h>
#include <pattern.h>
#include <assert.h>
#include <mime.h>


int Dir_find_file(const char *path, size_t path_len, size_t *out_size)
{
    int fd = 0;
    struct stat sb;

    // TODO: implement a stat cache and track inode changes in it
    int rc = stat(path, &sb);
    check(rc == 0, "File stat failed: %.*s", (int)path_len, path);

    *out_size = (size_t)sb.st_size;

    fd = open(path, O_RDONLY);
    check(fd, "Failed to open file but stat worked: %.*s", (int)path_len, path);

    return fd;

error:
    return -1;
}


int Dir_stream_file(int file_fd, size_t flen, int sock_fd)
{
    ssize_t sent = 0;
    size_t total = 0;
    off_t offset = 0;
    size_t block_size = MAX_SEND_BUFFER;

    for(total = 0; fdwait(sock_fd, 'w') == 0 && total < flen; total += sent) {
        sent = Dir_send(sock_fd, file_fd, &offset, block_size);
        check(sent > 0, "Failed to sendfile on socket: %d from file %d", sock_fd, file_fd);
    }

    check(total <= flen, "Wrote way too much, wrote %d but size was %d", (int)total, (int)flen);

    // TODO: with cache system in place we shouldn't close here
    close(file_fd);
    return sent;

error:

    // TODO: cache should have to do this on error
    close(file_fd);
    return -1;
}


Dir *Dir_create(const char *base)
{
    Dir *dir = calloc(sizeof(Dir), 1);
    check(dir, "Out of memory error.");

    dir->base_len = strlen(base);
    check(dir->base_len > 0, "Cannot have an empty directory base");
    check(dir->base_len < MAX_DIR_PATH, "Dir base cannot be greater than %d", MAX_DIR_PATH);

    strncpy(dir->base, base, MAX_DIR_PATH-1);
    dir->base[dir->base_len] = '\0';

    return dir;
error:
    return NULL;
}

void Dir_destroy(Dir *dir)
{
    free(dir);
}


int Dir_serve_file(Dir *dir, const char *path, int fd)
{
    // TODO: normalize path? probably doesn't matter since
    // we'll be chroot as a mandatory operation
    char header[512] = {0};
    const char *content_type = NULL;

    size_t path_len = strlen(path);
    size_t flen = 0;

    check(pattern_match(path, path_len, dir->base),
            "(len %d): Failed to match Dir base path: %.*s against PATH %.*s",
            (int)dir->base_len, 
            (int)dir->base_len, dir->base, (int)path_len, path);

    int file_fd = Dir_find_file(path+1, path_len-1, &flen);
    check(file_fd, "Error opening file: %.*s", (int)path_len, path);

    // TODO: get this from a configuration
    content_type = MIME_match_ext(path, path_len, "text/plain");


    int rc = snprintf(header, sizeof(header)-1, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", 
            content_type, flen);
    fdwrite(fd, header, rc);

    rc = Dir_stream_file(file_fd, flen, fd);
    check(rc == flen, "Didn't send all of the file, sent %d of %s.", rc, path);

    close(file_fd);
    close(fd);
    return 0;

error:
    close(file_fd);
    close(fd);
    return -1;
}

