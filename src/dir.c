#include <dir.h>
#include <fcntl.h>
#include <dbg.h>
#include <task/task.h>
#include <string.h>
#include <pattern.h>
#include <assert.h>
#include <mime.h>

struct tagbstring default_type = bsStatic ("text/plain");

const char *RESPONSE_FORMAT = "HTTP/1.1 200 OK\r\n"
    "Date: %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %d\r\n"
    "Last-Modified: %s\r\n"
    "ETag: %x-%x\r\n"
    "Connection: %s\r\n\r\n";

const char *RFC_822_TIME = "%a, %d %b %y %T %z";

enum {
    HOG_MAX = 1024
};

FileRecord *Dir_find_file(bstring path)
{
    FileRecord *fr = calloc(sizeof(FileRecord), 1);
    const char *p = bdata(path);

    check(fr, "Failed to make FileRecord memory.");

    int rc = stat(p, &fr->sb);
    check(rc == 0, "File stat failed: %s", bdata(path));

    fr->fd = open(p, O_RDONLY);
    check(fr->fd >= 0, "Failed to open file but stat worked: %s", bdata(path));

    fr->loaded = time(NULL);

    fr->date = bStrfTime(RFC_822_TIME, gmtime(&fr->loaded));
    check(fr->date, "Failed to format current date.");

    fr->last_mod = bStrfTime(RFC_822_TIME, gmtime(&fr->sb.st_mtime));
    check(fr->last_mod, "Failed to format last modified time.");

    // TODO: get this from a configuration
    fr->content_type = MIME_match_ext(path, &default_type);
    check(fr->content_type, "Should always get a content type back.");

    // don't let people who've received big files linger and hog the show
    const char *conn_close = fr->sb.st_size > HOG_MAX ? "close" : "keep-alive";

    fr->header = bformat(RESPONSE_FORMAT,
        bdata(fr->date),
        bdata(fr->content_type),
        fr->sb.st_size,
        bdata(fr->last_mod),
        fr->sb.st_mtime, fr->sb.st_size,
        conn_close);

    check(fr->header != NULL, "Failed to create response header.");

    return fr;

error:
    FileRecord_destroy(fr);
    return NULL;
}


int Dir_stream_file(FileRecord *file, int sock_fd)
{
    ssize_t sent = 0;
    size_t total = 0;
    off_t offset = 0;
    size_t block_size = MAX_SEND_BUFFER;

    fdwrite(sock_fd, bdata(file->header), blength(file->header));

    for(total = 0; fdwait(sock_fd, 'w') == 0 && total < file->sb.st_size; total += sent) {
        sent = Dir_send(sock_fd, file->fd, &offset, block_size);
        check(sent > 0, "Failed to sendfile on socket: %d from file %d", sock_fd, file->fd);
    }

    check(total <= file->sb.st_size, "Wrote way too much, wrote %d but size was %d", (int)total, (int)file->sb.st_size);

    return sent;

error:
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


void FileRecord_destroy(FileRecord *file)
{
    if(file) {
        fdclose(file->fd);
        bdestroy(file->date);
        bdestroy(file->last_mod);
        bdestroy(file->header);
        // file->content_type is not owned by us
        free(file);
    }
}


int Dir_serve_file(Dir *dir, bstring path, int fd)
{
    check(pattern_match(bdata(path), blength(path), bdata(dir->base)),
            "Failed to match Dir base path: %s against PATH %s",
            bdata(dir->base), bdata(path));

    FileRecord *file = Dir_find_file(path);
    check(file, "Error opening file: %s", bdata(path));

    int rc = Dir_stream_file(file, fd);
    check(rc == file->sb.st_size, "Didn't send all of the file, sent %d of %s.", rc, bdata(path));

    FileRecord_destroy(file);
    return 0;

error:
    FileRecord_destroy(file);
    return -1;
}

