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

#include <sys/stat.h>
#include "upload.h"
#include "dbg.h"
#include "setting.h"
#include "response.h"

bstring UPLOAD_STORE = NULL;
mode_t UPLOAD_MODE = 0;
struct tagbstring UPLOAD_MODE_DEFAULT = bsStatic("0600");

static inline int stream_to_disk(IOBuf *iob, int content_len, int tmpfd)
{
    char *data = NULL;
    int avail = 0;

    debug("max content length: %d, content_len: %d", MAX_CONTENT_LENGTH, content_len);

    IOBuf_resize(iob, MAX_CONTENT_LENGTH); // give us a good buffer size

    while(content_len > 0) {
        data = IOBuf_read_some(iob, &avail);
        check(!IOBuf_closed(iob), "Closed while reading from IOBuf.");
        content_len -= avail;
        check(write(tmpfd, data, avail) == avail, "Failed to write requested amount to tempfile: %d", avail);

        check(IOBuf_read_commit(iob, avail) != -1, "Final commit failed streaming to disk.");
    }

    check(content_len == 0, "Failed to write everything to the large upload tmpfile.");

    return 0;

error:
    return -1;
}


int Upload_notify(Connection *conn, Handler *handler, const char *stage, bstring tmp_name)
{
    bstring key = bformat("x-mongrel2-upload-%s", stage);
    Request_set(conn->req, key, bstrcpy(tmp_name), 1);

    return Connection_send_to_handler(conn, handler, "", 0);
}

int Upload_file(Connection *conn, Handler *handler, int content_len)
{
    int rc = 0;
    int tmpfd = 0;
    bstring tmp_name = NULL;
    bstring result = NULL;

    if(UPLOAD_STORE == NULL) {
        UPLOAD_STORE = Setting_get_str("upload.temp_store", NULL);
        error_unless(UPLOAD_STORE, conn, 413, "Request entity is too large: %d, and no upload.temp_store setting for where to put the big files.", content_len);

        UPLOAD_STORE = bstrcpy(UPLOAD_STORE);
    }

    if(UPLOAD_MODE == 0) {
        bstring mode = Setting_get_str("upload.temp_store_mode", &UPLOAD_MODE_DEFAULT);
        log_info("Will set mode for upload temp store to: %s", bdata(mode));

        UPLOAD_MODE = strtoul((const char *)mode->data, NULL, 0);
        check(UPLOAD_MODE > 0, "Failed to convert upload.temp_store_mode to a number.");
        check(UPLOAD_MODE < 066666, "Invalid mode that's way too big: %s.", bdata(mode));
    }

    tmp_name = bstrcpy(UPLOAD_STORE);

    tmpfd = mkstemp((char *)tmp_name->data);
    check(tmpfd != -1, "Failed to create secure tempfile, did you end it with XXXXXX?");

    log_info("Writing tempfile %s for large upload.", bdata(tmp_name));

    rc = chmod((char *)tmp_name->data, UPLOAD_MODE);
    check(rc == 0, "Failed to chmod.");

    rc = Upload_notify(conn, handler, "start", tmp_name);
    check(rc == 0, "Failed to notify of the start of upload.");

    rc = stream_to_disk(conn->iob, content_len, tmpfd);
    check(rc == 0, "Failed to stream to disk.");

    rc = Upload_notify(conn, handler, "done", tmp_name);
    check(rc == 0, "Failed to notify the end of the upload.");

    bdestroy(result);
    bdestroy(tmp_name);
    fdclose(tmpfd);
    return 0;

error:
    bdestroy(result);
    fdclose(tmpfd);

    if(tmp_name != NULL) {
        unlink((char *)tmp_name->data);
        bdestroy(tmp_name);
    }

    return -1;
}
