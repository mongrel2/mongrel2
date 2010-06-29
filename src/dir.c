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


int Dir_find_file(bstring path, size_t *out_size)
{
    int fd = 0;
    struct stat sb;
    const char *p = bdata(path);

    // TODO: implement a stat cache and track inode changes in it
    int rc = stat(p, &sb);
    check(rc == 0, "File stat failed: %s", bdata(path));

    *out_size = (size_t)sb.st_size;

    fd = open(p, O_RDONLY);
    check(fd, "Failed to open file but stat worked: %s", bdata(path));

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

    dir->base = bfromcstr(base);
    check(blength(dir->base) < MAX_DIR_PATH, "Base pattern is too long, must be less than %d", MAX_DIR_PATH);

    return dir;
error:
    return NULL;
}

void Dir_destroy(Dir *dir)
{
    bdestroy(dir->base);
    free(dir);
}

struct tagbstring default_type = bsStatic ("text/plain");


int Dir_serve_file(Dir *dir, bstring path, int fd)
{
    // TODO: normalize path? probably doesn't matter since
    // we'll be chroot as a mandatory operation

    size_t flen = 0;

    check(pattern_match(bdata(path), blength(path), bdata(dir->base)),
            "Failed to match Dir base path: %s against PATH %s",
            bdata(dir->base), bdata(path));

    int file_fd = Dir_find_file(path, &flen);

    check(file_fd, "Error opening file: %s", bdata(path));

    // TODO: get this from a configuration
    bstring content_type = MIME_match_ext(path, &default_type);

    bstring header = bformat("HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", 
            bdata(content_type), flen);
    check(header != NULL, "Failed to create response header.");

    fdwrite(fd, bdata(header), blength(header));

    int rc = Dir_stream_file(file_fd, flen, fd);
    check(rc == flen, "Didn't send all of the file, sent %d of %s.", rc, bdata(path));

    bdestroy(header);
    close(file_fd);
    close(fd);
    return 0;

error:
    bdestroy(header);
    close(file_fd);
    close(fd);
    return -1;
}

