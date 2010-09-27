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

#ifndef _dir_h
#define _dir_h

#include <stdlib.h>

#if defined(__APPLE__) || defined(__FreeBSD__)
#include "mac_specific.h"
#else
#include <sys/sendfile.h>
#endif
#include <bstring.h>
#include <cache.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <request.h>
#include <connection.h>

extern int MAX_SEND_BUFFER;
extern int MAX_DIR_PATH;

typedef struct FileRecord {
    int is_dir;
    int fd;
    int users;
    time_t loaded;
    bstring date;
    bstring last_mod;
    bstring content_type;
    bstring header;
    bstring request_path;
    bstring full_path;
    bstring etag;
    struct stat sb;
} FileRecord;

typedef struct Dir {
    Cache *fr_cache;
    bstring base;
    bstring normalized_base;
    bstring index_file;
    bstring default_ctype;
} Dir;

Dir *Dir_create(const char *base, const char *index_file,
        const char *default_ctype);

void Dir_destroy(Dir *dir);

FileRecord *Dir_find_file(bstring path, bstring default_type);

int Dir_stream_file(FileRecord *file, Connection *conn);

int Dir_serve_file(Dir *dir, Request *req, Connection *conn);

FileRecord *Dir_resolve_file(Dir *dir, bstring pattern, bstring path);

void FileRecord_release(FileRecord *file);
void FileRecord_destroy(FileRecord *file);

#if defined(__APPLE__) || defined(__FreeBSD__)
#define Dir_send mac_sendfile
#else
#define Dir_send sendfile
#endif

#define FR_CACHE_SIZE 32
#define FR_CACHE_TIME_TO_LIVE 10.0

#endif
