#include <dir.h>
#include <fcntl.h>
#include <dbg.h>
#include <task/task.h>
#include <string.h>
#include <pattern.h>
#include <assert.h>
#include <mime.h>
#include <response.h>


struct tagbstring ETAG_PATTERN = bsStatic("[a-e0-9]+-[a-e0-9]+");

const char *RESPONSE_FORMAT = "HTTP/1.1 200 OK\r\n"
    "Date: %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %d\r\n"
    "Last-Modified: %s\r\n"
    "ETag: %s\r\n"
    "Connection: keep-alive\r\n\r\n";

const char *RFC_822_TIME = "%a, %d %b %y %T %z";


FileRecord *Dir_find_file(bstring path, bstring default_type)
{
    FileRecord *fr = calloc(sizeof(FileRecord), 1);
    const char *p = bdata(path);

    check(fr, "Failed to make FileRecord memory.");

    // right here, if p ends with / then add index.html
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
    fr->content_type = MIME_match_ext(path, default_type);
    check(fr->content_type, "Should always get a content type back.");

    // we own this now, not the caller
    fr->full_path = path;

    fr->etag = bformat("%x-%x", fr->sb.st_mtime, fr->sb.st_size);

    fr->header = bformat(RESPONSE_FORMAT,
        bdata(fr->date),
        bdata(fr->content_type),
        fr->sb.st_size,
        bdata(fr->last_mod),
        bdata(fr->etag));

    check(fr->header != NULL, "Failed to create response header.");

    return fr;

error:
    FileRecord_destroy(fr);
    return NULL;
}


/** Mostly done for readability in the code, but later will be where header caching happens. */
inline int Dir_send_header(FileRecord *file, int sock_fd)
{
    return fdsend(sock_fd, bdata(file->header), blength(file->header)) == blength(file->header);
}

int Dir_stream_file(FileRecord *file, int sock_fd)
{
    ssize_t sent = 0;
    size_t total = 0;
    off_t offset = 0;
    size_t block_size = MAX_SEND_BUFFER;

    int rc = Dir_send_header(file, sock_fd);
    check(rc, "Failed to write header to socket.");

    for(total = 0; fdwait(sock_fd, 'w') == 0 && total < file->sb.st_size; total += sent) {
        sent = Dir_send(sock_fd, file->fd, &offset, block_size);
        check(sent > 0, "Failed to sendfile on socket: %d from file %d", sock_fd, file->fd);
    }

    check(total <= file->sb.st_size, "Wrote way too much, wrote %d but size was %d", (int)total, (int)file->sb.st_size);

    return sent;

error:
    return -1;
}


Dir *Dir_create(const char *base, const char *prefix, const char *index_file, const char *default_ctype)
{
    Dir *dir = calloc(sizeof(Dir), 1);
    check(dir, "Out of memory error.");

    dir->base = bfromcstr(base);
    check(blength(dir->base) < MAX_DIR_PATH, "Base direcotry is too long, must be less than %d", MAX_DIR_PATH);

    // dir can come from the routing table so it could have a pattern in it, strip that off
    bstring pattern = bfromcstr(prefix);
    int first_paren = bstrchr(pattern, '(');
    dir->prefix = first_paren >= 0 ? bHead(pattern, first_paren) : bstrcpy(pattern);
    bdestroy(pattern);

    check(blength(dir->prefix) < MAX_DIR_PATH, "Prefix is too long, must be less than %d", MAX_DIR_PATH);

    dir->index_file = bfromcstr(index_file);
    dir->default_ctype = bfromcstr(default_ctype);

    return dir;

error:
    return NULL;
}



void Dir_destroy(Dir *dir)
{
    if(dir) {
        bdestroy(dir->base);
        bdestroy(dir->prefix);
        bdestroy(dir->index_file);
        bdestroy(dir->normalized_base);
        bdestroy(dir->default_ctype);
        free(dir);
    }
}


void FileRecord_destroy(FileRecord *file)
{
    if(file) {
        fdclose(file->fd);
        bdestroy(file->date);
        bdestroy(file->last_mod);
        bdestroy(file->header);
        bdestroy(file->full_path);
        bdestroy(file->etag);
        // file->content_type is not owned by us
        free(file);
    }
}


inline int normalize_path(bstring target)
{
    ballocmin(target, PATH_MAX);

    char *normalized = realpath((const char *)target->data, NULL);
    check(normalized, "Failed to normalize path: %s", bdata(target));

    bassigncstr(target, normalized);
    free(normalized);

    return 0;

error:
    return 1;
}

inline int Dir_lazy_normalize_base(Dir *dir)
{
    if(dir->normalized_base == NULL) {
        dir->normalized_base = bstrcpy(dir->base);
        check(normalize_path(dir->normalized_base) == 0, 
                "Failed to normalize base path: %s", bdata(dir->normalized_base));
        debug("Lazy normalized base path %s into %s", 
                bdata(dir->base), bdata(dir->normalized_base));
    }
    return 0;

error:
    return -1;
}

FileRecord *Dir_resolve_file(Dir *dir, bstring path)
{
    FileRecord *file = NULL;
    bstring target = NULL;

    check(Dir_lazy_normalize_base(dir) == 0, "Failed to normalize base path.");

    check(bstrncmp(path, dir->prefix, blength(dir->prefix)) == 0, 
            "Request for path %s does not start with %s prefix.", 
            bdata(path), bdata(dir->prefix));

    if(bchar(path, blength(path) - 1) == '/') {
        target = bformat("%s%s/%s",
                    bdata(dir->normalized_base),
                    path->data + blength(dir->prefix),
                    bdata(dir->index_file));
    } else {
        target = bformat("%s/%s",
                bdata(dir->normalized_base),
                path->data + blength(dir->prefix));
    }

    check(target, "Couldn't construct target path for %s", bdata(path));

    check(normalize_path(target) == 0, "Failed to normalize target path.");
   
    check(bstrncmp(target, dir->normalized_base, blength(dir->normalized_base)) == 0, 
            "Request for path %s does not start with %s base after normalizing.", 
            bdata(target), bdata(dir->base));

    // the FileRecord now owns the target
    file = Dir_find_file(target, dir->default_ctype);
    check(file, "Error opening file: %s", bdata(target));

    return file;

error:
    FileRecord_destroy(file);
    return NULL;
}


inline bstring Dir_if_modified_since(Request *req, FileRecord *file, int if_modified_since)
{
    if(if_modified_since <= (int)time(NULL) && file->sb.st_mtime <= if_modified_since) {
        return &HTTP_304;
    } else {
        return NULL;
    }

    return &HTTP_500;
}

inline bstring Dir_none_match(Request *req, FileRecord *file, int if_modified_since, bstring if_none_match)
{
    if(biseqcstr(if_none_match, "*") || biseq(if_none_match, file->etag)) {
        return &HTTP_304;
    } else {
        if(if_modified_since) {
            return Dir_if_modified_since(req, file, if_modified_since);
        } else {
            return NULL;
        }
    }

    return &HTTP_500;
}




inline bstring Dir_calculate_response(Request *req, FileRecord *file)
{
    int if_unmodified_since = 0;
    int if_modified_since = 0;
    bstring if_match = NULL;
    bstring if_none_match = NULL;

    if(file) {
        if_match = Request_get(req, &HTTP_IF_MATCH);

        if(!if_match || biseqcstr(if_match, "*") || bstring_match(if_match, &ETAG_PATTERN)) {
            if_none_match = Request_get(req, &HTTP_IF_NONE_MATCH);
            if_unmodified_since = Request_get_date(req, &HTTP_IF_UNMODIFIED_SINCE, RFC_822_TIME);
            if_modified_since = Request_get_date(req, &HTTP_IF_MODIFIED_SINCE, RFC_822_TIME);

            debug("TESTING WITH: if_match: %s, if_none_match: %s, if_unmodified_since: %d, if_modified_since: %d",
                    bdata(if_match), bdata(if_none_match), if_unmodified_since, if_modified_since);

            if(if_unmodified_since) {
                if(file->sb.st_mtime > if_unmodified_since) {
                    return &HTTP_412;
                } else if(if_none_match) {
                    return Dir_none_match(req, file, if_modified_since, if_none_match);
                } else if(if_modified_since) {
                    return Dir_if_modified_since(req, file, if_modified_since);
                }
            } else if(if_none_match) {
                return Dir_none_match(req, file, if_modified_since, if_none_match);
            } else if(if_modified_since) {
                return Dir_if_modified_since(req, file, if_modified_since);
            } else {
                // they've got nothing, 200
                return NULL;
            }
        } else {
            return &HTTP_412;
        }
    } else {
        return &HTTP_404;
    }

    return &HTTP_500;
}


int Dir_serve_file(Request *req, Dir *dir, bstring path, int fd)
{
    FileRecord *file = NULL;
    bstring target = NULL;
    bstring resp = NULL;
    int rc = 0;
    int is_get = biseq(req->request_method, &HTTP_GET);
    int is_head = is_get ? 0 : biseq(req->request_method, &HTTP_HEAD);

    if(!(is_get || is_head)) {
        rc = Response_send_status(fd, &HTTP_405);
        check(rc == blength(&HTTP_405), "Failed to send 405 to client.");
        return -1;
    } else {
        file = Dir_resolve_file(dir, path);
        resp = Dir_calculate_response(req, file);

        if(resp) {
            rc = Response_send_status(fd, resp);
            check(rc == blength(resp), "Failed to send error response on file serving.");
        } else if(is_get) {
            rc = Dir_stream_file(file, fd);
            check(rc == file->sb.st_size, "Didn't send all of the file, sent %d of %s.", rc, bdata(target));
        } else if(is_head) {
            rc = Dir_send_header(file, fd);
            check(rc, "Failed to write header to socket.");
        } else {
            sentinel("How the hell did you get to here. Tell Zed.");
        }

        FileRecord_destroy(file);
        return 0;
    }

error:
    FileRecord_destroy(file);
    return -1;
}

