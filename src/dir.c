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
    "Content-Length: %d\r\n"
    "Last-Modified: %s\r\n"
    "ETag: %s\r\n"
    "Server: " VERSION
    "\r\n\r\n";

const char *DIR_REDIRECT_FORMAT = "HTTP/1.1 301 Moved Permanently\r\n"
    "Location: http://%s%s/\r\n"
    "Content-Length: 0\r\n"
    "Server: " VERSION
    "\r\n\r\n";

// TODO: confirm that we are actually doing the GMT time right
const char *RFC_822_TIME = "%a, %d %b %Y %H:%M:%S GMT";

static int filerecord_cache_lookup(void *data, void *key) {
    bstring request_path = (bstring) key;
    FileRecord *fr = (FileRecord *) data;
    
    return !bstrcmp(fr->request_path, request_path);
}

static void filerecord_cache_evict(void *data) {
    FileRecord_release((FileRecord *) data);
}


FileRecord *Dir_find_file(bstring path, bstring default_type)
{
    FileRecord *fr = calloc(sizeof(FileRecord), 1);
    const char *p = bdata(path);

    check_mem(fr);

    // We set the number of users here.  If we cache it, we can add one later
    fr->users = 1;

    int rc = stat(p, &fr->sb);
    check(rc == 0, "File stat failed: %s", bdata(path));

    if(S_ISDIR(fr->sb.st_mode)) {
        fr->full_path = path;
        fr->is_dir = 1;
        return fr;
    }
    
    fr->fd = open(p, O_RDONLY);
    check(fr->fd >= 0, "Failed to open file but stat worked: %s", bdata(path));

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

static inline int Dir_send_header(FileRecord *file, Connection *conn)
{
    return conn->send(conn, bdata(file->header), blength(file->header)) == blength(file->header);
}

int Dir_stream_file(FileRecord *file, Connection *conn)
{
    ssize_t sent = 0;
    size_t total = 0;
    off_t offset = 0;
    size_t block_size = MAX_SEND_BUFFER;

    // For the non-sendfile slowpath
    char *file_buffer = NULL;
    int nread = 0;
    int amt = 0;
    int tempfd = -1;

    int rc = Dir_send_header(file, conn);
    check_debug(rc, "Failed to write header to socket.");

    if(conn->ssl == NULL) {
        for(total = 0; fdwait(conn->fd, 'w') == 0 && total < file->sb.st_size;
            total += sent) {
            sent = Dir_send(conn->fd, file->fd, &offset, block_size);
            check_debug(sent > 0, "Failed to sendfile on socket: %d from "
                        "file %d", conn->fd, file->fd);
        }
    }
    else {
        // We have to reopen the file, so we don't get ourselves into seek
        // position trouble
        int tempfd = open(bdata(file->full_path), O_RDONLY);
        check(tempfd >= 0, "Could not reopen open file");

        file_buffer = malloc(MAX_SEND_BUFFER);
        check_mem(file_buffer);

        while((nread = fdread(tempfd, file_buffer, MAX_SEND_BUFFER)) > 0) {
            for(amt = 0, sent = 0; sent < nread; sent += amt) {
                amt = conn->send(conn, file_buffer + sent, nread - sent);
                check_debug(amt > 0, "Failed to send on socket: %d from "
                            "file %d", conn->fd, tempfd);
            }
            total += nread;
        }
        free(file_buffer);
        close(tempfd); tempfd = -1;
    }
    
    check(total <= file->sb.st_size, 
            "Wrote way too much, wrote %d but size was %d",
            (int)total, (int)file->sb.st_size);

    check(total == file->sb.st_size,
            "Sent other than expected, sent: %d, but expected: %d", 
            (int)total, (int)file->sb.st_size);

    return total;

error:
    if(file_buffer) free(file_buffer);
    if(tempfd >= 0) close(tempfd);
    return -1;
}


Dir *Dir_create(const char *base, const char *prefix, const char *index_file, const char *default_ctype)
{
    Dir *dir = calloc(sizeof(Dir), 1);
    check_mem(dir);

    if(!MAX_SEND_BUFFER || !MAX_DIR_PATH) {
        MAX_SEND_BUFFER = Setting_get_int("limits.dir_send_buffer", 16 * 1024);
        MAX_DIR_PATH = Setting_get_int("limits.dir_max_path", 256);
        log_info("MAX limits.dir_send_buffer=%d, limits.dir_max_path=%d",
                MAX_SEND_BUFFER, MAX_DIR_PATH);
    }

    dir->base = bfromcstr(base);
    check(blength(dir->base) < MAX_DIR_PATH, "Base directory is too long, must be less than %d", MAX_DIR_PATH);
    check(bchar(dir->base, 0) != '/', "Don't start the base with / in %s, that will fail when not in chroot.", base);
    check(bchar(dir->base, blength(dir->base) - 1) == '/', "End directory base with / in %s or it won't work right.", base);


    // dir can come from the routing table so it could have a pattern in it, strip that off
    bstring pattern = bfromcstr(prefix);
    int first_paren = bstrchr(pattern, '(');
    dir->prefix = first_paren >= 0 ? bHead(pattern, first_paren) : bstrcpy(pattern);
    bdestroy(pattern);

    check(blength(dir->prefix) < MAX_DIR_PATH, "Prefix is too long, must be less than %d", MAX_DIR_PATH);

    check(bchar(dir->prefix, 0) == '/', "Dir route prefix (%s) must start with / or else it won't work.", bdata(dir->prefix));

    if(bchar(dir->prefix, blength(dir->prefix)-1) != '/') {
        log_info("Dir prefix %s doesn't end in / so assuming it's a file.", bdata(dir->prefix));
    }

    dir->index_file = bfromcstr(index_file);
    dir->default_ctype = bfromcstr(default_ctype);

    dir->fr_cache = Cache_create(FR_CACHE_SIZE, filerecord_cache_lookup,
                                 filerecord_cache_evict);
    check(dir->fr_cache, "Failed to create FileRecord cache");

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
        bdestroy(dir->prefix);
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
            fdclose(file->fd);
            bdestroy(file->date);
            bdestroy(file->last_mod);
            bdestroy(file->header);
            bdestroy(file->etag);
        }
        bdestroy(file->full_path);
        // file->content_type is not owned by us
        free(file);
    }
}


static inline int normalize_path(bstring target)
{
    ballocmin(target, PATH_MAX);
    char *path_buf = calloc(PATH_MAX+1, 1);

    // Some platforms (OSX!) don't allocate for you, so we have to
    char *normalized = realpath((const char *)target->data, path_buf);
    check(normalized, "Failed to normalize path: %s", bdata(target));

    bassigncstr(target, normalized);
    free(path_buf);

    return 0;

error:
    return 1;
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

        if(difftime(now, file->loaded) > FR_CACHE_TIME_TO_LIVE) {
            int rcstat = stat(p, &sb);

            if(rcstat != 0 || file->sb.st_mtime != sb.st_mtime || file->sb.st_size != sb.st_size) {
                Cache_evict_object(dir->fr_cache, file);
                file = NULL;
            } else {
                file->loaded = now;
            }
        }
    }

    return file;
}


FileRecord *Dir_resolve_file(Dir *dir, bstring path)
{
    FileRecord *file = NULL;
    bstring target = NULL;

    check(Dir_lazy_normalize_base(dir) == 0, "Failed to normalize base path when requesting %s",
            bdata(path));

    check(bstrncmp(path, dir->prefix, blength(dir->prefix)) == 0, 
            "Request for path %s does not start with %s prefix.", 
            bdata(path), bdata(dir->prefix));

    file = FileRecord_cache_check(dir, path);

    if(file) {
        // TODO: double check this gives the right users count
        file->users++;
        return file;
    }

    // We subtract one from the blengths below, because dir->prefix includes
    // a trailing '/'.  If we skip over this in path->data, we drop the '/'
    // from the URI, breaking the target path
    debug("Building target from base: %s prefix: %s index_file: %s path: %s", 
            bdata(dir->normalized_base),
            bdata(dir->prefix),
            bdata(dir->index_file),
            bdata(path));

    if(bchar(path, blength(path) - 1) == '/') {
        // a directory so figureo out the index file
        target = bformat("%s%s%s",
                    bdata(dir->normalized_base),
                    path->data + blength(dir->prefix) - 1,
                    bdata(dir->index_file));
    } else if(biseq(dir->prefix, path)) {
        // a full path to a file that matches the Dir prefix as a file
        // TODO: optimize this somewhat and make sure it doesn't make weird paths
        target = bformat("%s%s", bdata(dir->normalized_base), bdata(path)); 
    } else {
        // all other requests for files
        target = bformat("%s%s",
                bdata(dir->normalized_base),
                path->data + blength(dir->prefix) - 1);
    }

    check(target, "Couldn't construct target path for %s", bdata(path));

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
    int rc = 0;
    int is_get = biseq(req->request_method, &HTTP_GET);
    int is_head = is_get ? 0 : biseq(req->request_method, &HTTP_HEAD);

    check(path, "Request had not path. That's weird.");
    req->response_size = 0;

    if(!(is_get || is_head)) {
        req->status_code = 405;
        rc = Response_send_status(conn, &HTTP_405);
        check_debug(rc == blength(&HTTP_405), "Failed to send 405 to client.");
        return -1;
    } else {
        file = Dir_resolve_file(dir, path);
        resp = Dir_calculate_response(req, file);

        if(resp) {
            rc = Response_send_status(conn, resp);
            check_debug(rc == blength(resp), "Failed to send error response on file serving.");
        } else if(is_get) {
            rc = Dir_stream_file(file, conn);
            req->response_size = rc;
            check_debug(rc == file->sb.st_size, "Didn't send all of the file, sent %d of %s.", rc, bdata(path));
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

