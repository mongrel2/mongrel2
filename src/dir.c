#include <dir.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dbg.h>
#include <task/task.h>

int Dir_find_file(const char *path, size_t path_len, size_t *out_size)
{
    struct stat sb;
    int fd = 0;

    // TODO: implement a stat cache and track inode changes in it
    int rc = stat(path, &sb);
    check(rc == 0, "File stat failed: %.*s", path_len, path);

    *out_size = (size_t)sb.st_size;

    fd = open(path, O_RDONLY);
    check(fd, "Failed to open file but stat worked: %.*s", path_len, path);

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

    check(total <= flen, "Wrote way too much, wrote %d but size was %d", total, flen);

    // TODO: with cache system in place we shouldn't close here
    close(file_fd);
    return sent;

error:

    // TODO: cache should have to do this on error
    close(file_fd);
    return -1;
}

