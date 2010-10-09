#include "upload.h"
#include "dbg.h"
#include "setting.h"
#include "response.h"



bstring Upload_file(Connection *conn, Handler *handler, int content_len)
{
    int rc = 0;
    int tmpfd = 0;
    bstring result = NULL;
    char *data = NULL;

    // need a setting for the moby store where large uploads should go
    bstring upload_store = Setting_get_str("upload.temp_store", NULL);
    error_unless(upload_store, conn, 413, "Request entity is too large: %d, and no upload.temp_store setting for where to put the big files.", content_len);

    upload_store = bstrcpy(upload_store); // Setting owns the original

    // TODO: handler should be set to allow large uploads, otherwise error

    tmpfd = mkstemp((char *)upload_store->data);
    check(tmpfd != -1, "Failed to create secure tempfile, did you end it with XXXXXX?");
    log_info("Writing tempfile %s for large upload.", bdata(upload_store));

    // send the initial headers we have so they can kill it if they want
    dict_alloc_insert(conn->req->headers, bfromcstr("X-Mongrel2-Upload-Start"), upload_store);

    result = Request_to_payload(conn->req, handler->send_ident,
            IOBuf_fd(conn->iob), "", 0);
    check(result, "Failed to create initial payload for upload attempt.");

    rc = Handler_deliver(handler->send_socket, bdata(result), blength(result));
    check(rc != -1, "Failed to deliver upload attempt to handler.");

    // all good so start streaming chunks into the temp file in the moby dir
    IOBuf_resize(conn->iob, MAX_CONTENT_LENGTH); // give us a good buffer size

    while(content_len > 0) {
        data = IOBuf_read_some(conn->iob, &rc);
        check(!IOBuf_closed(conn->iob), "Closed while reading from IOBuf.");
        content_len -= rc;

        check(write(tmpfd, data, rc) == rc, "Failed to write requested amount to tempfile: %d", rc);

        IOBuf_read_commit(conn->iob, rc);
    }

    check(content_len == 0, "Bad math on writing out the upload tmpfile: %s, it's %d", bdata(upload_store), content_len);

    // moby dir write is done, add a header to the request that indicates where to get it
    dict_alloc_insert(conn->req->headers, bfromcstr("X-Mongrel2-Upload-Done"), upload_store);

    bdestroy(result);
    fdclose(tmpfd);
    return upload_store;

error:
    bdestroy(result);
    fdclose(tmpfd);

    if(upload_store) {
        unlink((char *)upload_store->data);
        bdestroy(upload_store);
    }

    return NULL;
}
