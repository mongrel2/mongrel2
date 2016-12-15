/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#undef NDEBUG

#include <dir.h>
#include <cache.h>
#include <fcntl.h>
#include <dbg.h>
#include <task/task.h>
#include <string.h>
#include <pattern.h>
#include <assert.h>
#include <mime.h>
#include <response.h>
#include "version.h"
#include "setting.h"

int MAX_DIR_PATH = 0;
int MAX_SEND_BUFFER = 0;

struct tagbstring ETAG_PATTERN = bsStatic("[a-e0-9]+-[a-e0-9]+");

const char *RESPONSE_FORMAT = "HTTP/1.1 200 OK\r\n"
    "Date: %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %llu\r\n"
    "Last-Modified: %s\r\n"
    "ETag: %s\r\n"
    "Server: " VERSION
    "\r\n\r\n";

const char *DIR_REDIRECT_FORMAT = "HTTP/1.1 301 Moved Permanently\r\n"
    "Location: //%s%s/\r\n"
    "Content-Length: 0\r\n"
    "Server: " VERSION
    "\r\n\r\n";

// TODO: confirm that we are actually doing the GMT time right
static const char *RFC_822_TIME = "%a, %d %b %Y %H:%M:%S GMT";

static int filerecord_cache_lookup(void *data, void *key) {
    bstring request_path = (bstring) key;
    FileRecord *fr = (FileRecord *) data;

    return !bstrcmp(fr->request_path, request_path);
}

static void filerecord_cache_evict(void *data) {
    FileRecord_release((FileRecord *) data);
}

static inline int get_file_real_size(FileRecord *fr)
{
    // TODO: this is the total suck we'll redesign this away
    int fd = open(bdatae(fr->full_path,""), O_RDONLY);
    check(fd >= 0, "Failed to open file but stat worked: %s", bdata(fr->full_path));

    fr->file_size = lseek(fd, 0L, SEEK_END);
    check(fr->file_size >= 0, "Failed to seek end of file: %s", bdata(fr->full_path));
    lseek(fd, 0L, SEEK_SET);

    fdclose(fd);

    return 0;
error:
    if (fd>=0) {
        fdclose(fd);
    }
    return -1;
}



FileRecord *Dir_find_file(bstring path, bstring default_type)
{
    FileRecord *fr = calloc(sizeof(FileRecord), 1);

    check_mem(fr);

    // We set the number of users here.  If we cache it, we can add one later
    fr->users = 1;
    fr->full_path = path;

    int rc = stat(bdatae(fr->full_path,""), &fr->sb);
    check(rc == 0, "File stat failed: %s", bdata(fr->full_path));

    if(S_ISDIR(fr->sb.st_mode)) {
        fr->is_dir = 1;
        return fr;
    }

    check(get_file_real_size(fr) == 0, "Failed to setup the file record for %s", bdata(fr->full_path));
    fr->loaded = time(NULL);

    fr->last_mod = bStrfTime(RFC_822_TIME, gmtime(&fr->sb.st_mtime));
    check(fr->last_mod, "Failed to format last modified time.");

    // TODO: get this from a configuration
    fr->content_type = MIME_match_ext(path, default_type);
    check(fr->content_type, "Should always get a content type back.");

    // we own this now, not the caller
    fr->full_path = path;

    time_t now = time(NULL);

    fr->date = bStrfTime(RFC_822_TIME, gmtime(&now));

    fr->etag = bformat("%x-%x", fr->sb.st_mtime, fr->file_size);

    fr->header = bformat(RESPONSE_FORMAT,
        bdata(fr->date),
        bdata(fr->content_type),
        fr->file_size,
        bdata(fr->last_mod),
        bdata(fr->etag));

    check(fr->header != NULL, "Failed to create response header.");

    return fr;

error:
    FileRecord_destroy(fr);
    return NULL;
}

static inline int Dir_send_header(FileRecord *file, Connection *conn)
{
    return IOBuf_send(conn->iob, bdata(file->header), blength(file->header));
}

long long int Dir_stream_file(FileRecord *file, Connection *conn)
{
    long long int sent = 0;
    int fd = -1;

    int rc = Dir_send_header(file, conn);
    check_debug(rc, "Failed to write header to socket.");

    fd = open(bdatae(file->full_path,""), O_RDONLY);
    check(fd >= 0, "Failed to open file: %s", bdata(file->full_path));

    sent = IOBuf_stream_file(conn->iob, fd, file->file_size);
    check(sent == file->file_size, "Error streaming file. Sent %d of %d bytes.", sent, file->file_size);

    fdclose(fd);
    return file->file_size;

error:
    if(fd >= 0) fdclose(fd);
    return -1;
}


Dir *Dir_create(bstring base, bstring index_file, bstring default_ctype, int cache_ttl)
{
    Dir *dir = calloc(sizeof(Dir), 1);
    check_mem(dir);

    dir->running = 1;

    if(!MAX_SEND_BUFFER || !MAX_DIR_PATH) {
        MAX_SEND_BUFFER = Setting_get_int("limits.dir_send_buffer", 16 * 1024);
        MAX_DIR_PATH = Setting_get_int("limits.dir_max_path", 256);
        log_info("MAX limits.dir_send_buffer=%d, limits.dir_max_path=%d",
                MAX_SEND_BUFFER, MAX_DIR_PATH);
    }

    dir->base = bstrcpy(base);
    check(blength(dir->base) < MAX_DIR_PATH, "Base directory is too long, must be less than %d", MAX_DIR_PATH);
    check(bchar(dir->base, blength(dir->base) - 1) == '/', "End directory base with / in %s or it won't work right.", bdata(base));

    dir->index_file = bstrcpy(index_file);
    dir->default_ctype = bstrcpy(default_ctype);

    dir->fr_cache = Cache_create(FR_CACHE_SIZE, filerecord_cache_lookup,
                                 filerecord_cache_evict);
    check(dir->fr_cache, "Failed to create FileRecord cache");

    check(cache_ttl >= 0, "Invalid cache ttl, must be a positive integer");
    dir->cache_ttl = cache_ttl;

    return dir;

error:
    if(dir)
        free(dir);

    return NULL;
}



void Dir_destroy(Dir *dir)
{
    if(dir) {
        bdestroy(dir->base);
        bdestroy(dir->index_file);
        bdestroy(dir->normalized_base);
        bdestroy(dir->default_ctype);
        if(dir->fr_cache) Cache_destroy(dir->fr_cache);
        free(dir);
    }
}

void FileRecord_release(FileRecord *file)
{
    if(file) {
        file->users--;
        check(file->users >= 0, "User count on file record somehow fell below 0");
        if(file->users <= 0) FileRecord_destroy(file);
    }

error:
    return;
}

void FileRecord_destroy(FileRecord *file)
{
    if(file) {
        if(!file->is_dir) {
            bdestroy(file->date);
            bdestroy(file->last_mod);
            bdestroy(file->header);
            bdestroy(file->etag);
            bdestroy(file->request_path);
        }
        bdestroy(file->full_path);
        // file->content_type is not owned by us
        free(file);
    }
}

static inline void burl_decode(bstring b)
{
    char d1; /* will contain candidate for 1st digit */
    char d2; /* will contain candidate for 2nd digit */
    char *cur = bdata(b);
    char *out = bdata(b);
    char *end = cur+blength(b);

    if(blength(b) == 0) {
        return;
    }

    while(cur < end) {
        /* One character left in input */
        if (cur + 1 == end) {
            *out = *cur;
            *(out+1) = '\0';
            btrunc(b,out + 1 - bdata(b));
            return;
        }
        d1 = *(cur+1);

        /* Two characters left in input */
        if (cur + 2 == end) {
            *out = *cur;
            *(out+1) = *(cur+1);
            *(out+2) = '\0';
            btrunc(b,out + 2 - bdata(b));
            return;
        }

        d2 = *(cur+2);

        /* Legal escape sequence */
        if(*cur=='%' && isxdigit(d1) && isxdigit(d2)) {
            d1 = tolower(d1);
            d2 = tolower(d2);

            if( d1 <= '9' )
                d1 = d1 - '0';
            else
                d1 = d1 - 'a' + 10;
            if( d2 <= '9' )
                d2 = d2 - '0';
            else
                d2 = d2 - 'a' + 10;

            *out = 16 * d1 + d2;

            out += 1;
            cur += 3;
        }
        else {
            *out = *cur;
            out += 1;
            cur += 1;
        }
    }

    check(0, "Bug in burl_decode: unreachable line reached");
error:
    btrunc(b,out - bdata(b));
    return;
}

/* realpath() in older versions of POSIX was ill-specified if PATH_MAX is not
 * defined.  Newer versions of POSIX allow passing NULL as the second argument
 * so the most portable thing to do is to use a buffer if PATH_MAX is defined
 * and NULL if it is not defined. */
static bstring brealpath(bstring target)
{
#ifdef PATH_MAX
    bstring returnme = bfromcstralloc(PATH_MAX+1,"X");
    bpattern(returnme,PATH_MAX);
    char *normalized = realpath((const char *)(bdata(target)),bdata(returnme));
    check_debug(normalized, "Failed to normalize path: %s %d %s", bdata(target), errno, strerror(errno));
    //btrunc(returnme,strlen(normalized));
    btrunc(returnme,strlen(bdata(returnme)));
#else
    bstring returnme = NULL;
    char *normalized = realpath((const char *)(bdata(target)),NULL);
    check_debug(normalized, "Failed to normalize path: %s %d %s", bdata(target), errno, strerror(errno));

    returnme = bfromcstr(normalized);

    free(normalized);
#endif
    return returnme;

error:
    bdestroy(returnme);
    return NULL;
}


static inline int normalize_path(bstring target)
{


    burl_decode(target);



    bstring normalized = brealpath(target);


    check_debug(normalized, "Failed to normalize path: %s", bdata(target));

    check(BSTR_OK == bassign(target, normalized), "Failed to assign target");

    bdestroy(normalized);


    return 0;

error:
    return -1;
}

static inline int Dir_lazy_normalize_base(Dir *dir)
{
    if(dir->normalized_base == NULL) {
        dir->normalized_base = bstrcpy(dir->base);
        check(normalize_path(dir->normalized_base) == 0,
            "Failed to normalize base path: %s", bdata(dir->normalized_base));

        debug("Lazy normalized base path %s into %s", bdata(dir->base), bdata(dir->normalized_base));
    }
    return 0;

error:
    return -1;
}

FileRecord *FileRecord_cache_check(Dir *dir, bstring path)
{
    FileRecord *file = Cache_lookup(dir->fr_cache, path);

    if(file) {
        time_t now = time(NULL);
        const char *p = bdata(file->full_path);
        struct stat sb;

        if(difftime(now, file->loaded) > dir->cache_ttl) {
            if( p == NULL ||
                0 != stat(p, &sb) ||
                file->sb.st_mtime != sb.st_mtime ||
                file->sb.st_ctime != sb.st_ctime ||
                file->sb.st_uid != sb.st_uid ||
                file->sb.st_gid != sb.st_gid ||
                file->sb.st_mode != sb.st_mode ||
                file->sb.st_size != sb.st_size ||
                file->sb.st_ino != sb.st_ino ||
                file->sb.st_dev != sb.st_dev 
            ) {
                Cache_evict_object(dir->fr_cache, file);
                file = NULL;
            } else {
                file->loaded = now;
            }
        }
    }

    return file;
}


FileRecord *Dir_resolve_file(Dir *dir, bstring prefix, bstring path)
{
    FileRecord *file = NULL;
    bstring target = NULL;

    check(blength(prefix) <= blength(path), 
            "Path '%s' is shorter than prefix '%s', not allowed.", bdata(path), bdata(prefix));

    check(Dir_lazy_normalize_base(dir) == 0, "Failed to normalize base path when requesting %s",
            bdata(path));

    file = FileRecord_cache_check(dir, path);

    if(file) {
        // TODO: double check this gives the right users count
        file->users++;
        return file;
    }

    check(bchar(prefix, 0) == '/', "Route '%s' pointing to directory must have prefix with leading '/'", bdata(prefix));
    check(blength(prefix) < MAX_DIR_PATH, "Prefix is too long, must be less than %d", MAX_DIR_PATH);

    debug("Building target from base: %s prefix: %s path: %s index_file: %s",
            bdata(dir->normalized_base),
            bdata(prefix),
            bdata(path),
            bdata(dir->index_file));

    if(bchar(path, blength(path) - 1) == '/') {
        // a directory so figure out the index file
        target = bformat("%s/%s%s",
                         bdata(dir->normalized_base),
                         bdataofs(path, blength(prefix)),
                         bdata(dir->index_file));
    } else if(biseq(prefix, path)) {
        target = bformat("%s%s", bdata(dir->normalized_base), bdata(path));
    } else {
        target = bformat("%s/%s", bdata(dir->normalized_base), bdataofs(path, blength(prefix)));
    }

    check_mem(target);

    check_debug(normalize_path(target) == 0,
            "Failed to normalize target path: %s", bdata(target));

    check_debug(bstrncmp(target, dir->normalized_base, blength(dir->normalized_base)) == 0,
            "Request for path %s does not start with %s base after normalizing.",
            bdata(target), bdata(dir->base));

    // the FileRecord now owns the target
    file = Dir_find_file(target, dir->default_ctype);
    check_debug(file, "Error opening file: %s", bdata(target));

    // Increment the user count because we're adding it to the cache
    file->users++;
    file->request_path = bstrcpy(path);
    Cache_add(dir->fr_cache, file);

    return file;

error:
    bdestroy(target);
    FileRecord_release(file);
    return NULL;
}


static inline bstring Dir_if_modified_since(Request *req, FileRecord *file, int if_modified_since)
{
    if(if_modified_since <= (int)time(NULL) && file->sb.st_mtime <= if_modified_since) {
        req->status_code = 304;
        return &HTTP_304;
    } else {
        return NULL;
    }

    req->status_code = 500;
    return &HTTP_500;
}

static inline bstring Dir_none_match(Request *req, FileRecord *file, int if_modified_since, bstring if_none_match)
{
    if(biseqcstr(if_none_match, "*") || biseq(if_none_match, file->etag)) {
        req->status_code = 304;
        return &HTTP_304;
    } else {
        if(if_modified_since) {
            return Dir_if_modified_since(req, file, if_modified_since);
        } else {
            return NULL;
        }
    }

    req->status_code = 500;
    return &HTTP_500;
}

static inline bstring Dir_calculate_response(Request *req, FileRecord *file)
{
    int if_unmodified_since = 0;
    int if_modified_since = 0;
    bstring if_match = NULL;
    bstring if_none_match = NULL;

    if(file) {
        if(file->is_dir)
            return bformat(DIR_REDIRECT_FORMAT, bdata(req->host),
                           bdata(req->uri));

        if_match = Request_get(req, &HTTP_IF_MATCH);

        if(!if_match || biseqcstr(if_match, "*") || bstring_match(if_match, &ETAG_PATTERN)) {
            if_none_match = Request_get(req, &HTTP_IF_NONE_MATCH);
            if_unmodified_since = Request_get_date(req, &HTTP_IF_UNMODIFIED_SINCE, RFC_822_TIME);
            if_modified_since = Request_get_date(req, &HTTP_IF_MODIFIED_SINCE, RFC_822_TIME);

            debug("TESTING WITH: if_match: %s, if_none_match: %s, if_unmodified_since: %d, if_modified_since: %d",
                    bdata(if_match), bdata(if_none_match), if_unmodified_since, if_modified_since);

            if(if_unmodified_since) {
                if(file->sb.st_mtime > if_unmodified_since) {
                    req->status_code = 412;
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
                req->status_code = 200;
                return NULL;
            }
        } else {
            req->status_code = 412;
            return &HTTP_412;
        }
    } else {
        req->status_code = 404;
        return &HTTP_404;
    }

    req->status_code = 500;
    return &HTTP_500;
}

int Dir_serve_file(Dir *dir, Request *req, Connection *conn)
{
    FileRecord *file = NULL;
    bstring resp = NULL;
    bstring path = Request_path(req);
    bstring prefix = req->prefix;
    check(prefix != NULL, "Request without a prefix hit.");
    check(dir->running, "Directory is not running anymore.");

    long long int rc = 0;
    int is_get = biseq(req->request_method, &HTTP_GET);
    int is_head = is_get ? 0 : biseq(req->request_method, &HTTP_HEAD);

    check(path, "Request had not path. That's weird.");
    req->response_size = 0;

    if(!(is_get || is_head)) {
        req->status_code = 405;
        rc = Response_send_status(conn, &HTTP_405);
        check_debug(rc == blength(&HTTP_405), "Failed to send 405 to client.");
        return -1;
    } else if (blength(prefix) > blength(path)) {
        req->status_code = 404;
        rc = Response_send_status(conn, &HTTP_404);
        check_debug(rc == blength(&HTTP_404), "Failed to send 404 to client.");
        return -1;
    } else {
        file = Dir_resolve_file(dir, prefix, path);
        resp = Dir_calculate_response(req, file);

        if(resp) {
            rc = Response_send_status(conn, resp);
            check_debug(rc == blength(resp), "Failed to send error response on file serving.");
        } else if(is_get) {
            rc = Dir_stream_file(file, conn);
            req->response_size = rc;
            check_debug(rc == file->file_size, "Didn't send all of the file, sent %lld of %s.", rc, bdata(path));
        } else if(is_head) {
            rc = Dir_send_header(file, conn);
            check_debug(rc, "Failed to write header to socket.");
        } else {
            sentinel("How the hell did you get to here. Tell Zed.");
        }

        FileRecord_release(file);
        return 0;
    }

    sentinel("Invalid code branch, Tell Zed you have magic.");
error:
    FileRecord_release(file);
    return -1;
}

