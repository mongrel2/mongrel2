#undef NDEBUG

#include "io.h"
#include "mem/halloc.h"
#include "dbg.h"
#include <stdlib.h>
#include <assert.h>


static ssize_t plaintext_send(IOBuf *iob, char *buffer, int len)
{
    return fdsend(iob->fd, buffer, len);
}

static ssize_t plaintext_recv(IOBuf *iob, char *buffer, int len)
{
    return fdrecv(iob->fd, buffer, len);
}

static ssize_t file_send(IOBuf *iob, char *buffer, int len)
{
    return fdwrite(iob->fd, buffer, len);    
}

static ssize_t file_recv(IOBuf *iob, char *buffer, int len)
{
    return fdread(iob->fd, buffer, len);
}


static ssize_t ssl_send(IOBuf *iob, char *buffer, int len)
{
    check(iob->ssl != NULL, "Cannot ssl_send on a iobection without ssl");
    
    return ssl_write(iob->ssl, (const unsigned char*) buffer, len);

error:
    return -1;
}

static ssize_t ssl_recv(IOBuf *iob, char *buffer, int len)
{
    check(iob->ssl != NULL, "Cannot ssl_recv on a iobection without ssl");
    unsigned char **pread = NULL;
    int nread;

    // If we didn't read all of what we recieved last time
    if(iob->ssl_buf != NULL) {
        if(iob->ssl_buf_len < len) {
            nread = iob->ssl_buf_len;
            *pread = (unsigned char *)iob->ssl_buf;
            iob->ssl_buf_len = 0;
            iob->ssl_buf = NULL;
        }
        else {
            nread = len;
            *pread = (unsigned char *)iob->ssl_buf;
            iob->ssl_buf += nread;
            iob->ssl_buf_len -= nread;
        }
    }
    else {
        do {
            nread = ssl_read(iob->ssl, pread);
        } while(nread == SSL_OK);

        // If we got more than they asked for, we should stash it for
        // successive calls.
        if(nread > len) {
            iob->ssl_buf = buffer + len;
            iob->ssl_buf_len = nread - len;
            nread = len;
        }
    }
    if(nread < 0) 
        return nread;

    check(*pread != NULL, "Got a NULL from ssl_read despite no error code");
    
    memcpy(buffer, *pread, nread);
    return nread;

error:
    return -1;
}


IOBuf *IOBuf_create(size_t len, int fd, IOBufType type)
{
    IOBuf *buf = h_calloc(sizeof(IOBuf), 1);
    check_mem(buf);

    buf->fd = fd;
    buf->len = len;

    buf->buf = h_malloc(len);
    check_mem(buf->buf);

    hattach(buf->buf, buf);

    buf->type = type;

    if(type == IOBUF_SSL) {
        buf->send = ssl_send;
        buf->recv = ssl_recv;
    } else if(type == IOBUF_FILE) {
        buf->send = file_send;
        buf->recv = file_recv;
    } else if(type == IOBUF_SOCKET) {
        buf->send = plaintext_send;
        buf->recv = plaintext_recv;
    } else {
        sentinel("Invalid IOBufType given: %d", type);
    }

    return buf;

error:
    if(buf) h_free(buf);
    return NULL;
}

void IOBuf_destroy(IOBuf *buf)
{
    fdclose(buf->fd);
    if(buf->ssl) ssl_free(buf->ssl);
    h_free(buf);
}

void IOBuf_resize(IOBuf *buf, size_t new_size)
{
    buf->buf = h_realloc(buf->buf, new_size);
    buf->len = new_size;
}


static inline void IOBuf_compact(IOBuf *buf)
{
    debug("Compacting buffer down.");
    memmove(buf->buf, IOBuf_start(buf), buf->avail);
    buf->cur = 0;
}

char *IOBuf_read(IOBuf *buf, int need, int *out_len)
{
    int rc = 0;
    assert(buf->cur + buf->avail <= buf->len && 
            "Buffer math off, cur+avail can't be more than > len.");
    assert(buf->cur >= 0 && "Buffer cur can't be < 0");
    assert(buf->avail >= 0 && "Buffer avail can't be < 0");
    assert(need < buf->len && "Request for more than possible in the buffer.");

    if(buf->avail < need) {
        debug("need to fill the buffer with more, attempt a read");
        if(buf->cur > 0 && IOBuf_compact_needed(buf, need)) {
            IOBuf_compact(buf);
        }

        debug("Attempting to read as much as possible: remain: %d, avail: %d", 
                IOBuf_remaining(buf), buf->avail);
        rc = buf->recv(buf, IOBuf_read_point(buf), IOBuf_remaining(buf));

        if(rc < 0) {
            debug("Socket was closed, will return only what's available: %d", buf->avail);
            buf->closed = 1;
        }

        buf->avail = buf->avail + rc;
        debug("New available: %d", buf->avail);

        if(buf->avail < need) {
            // less than expected
            *out_len = buf->avail;
        } else {
            // more than expected
            *out_len = need;
        }
    } else {
        debug("we have enough to satisfy the request to return it");
        *out_len = need;
    }

    debug("Out length: %d", *out_len);
    assert(buf->avail >= need && "Don't have enough even after trying to fill.");

    // now everything should be good to go from cur point
    return IOBuf_start(buf);
}


void IOBuf_read_commit(IOBuf *buf, int need)
{
    buf->avail -= need;
    assert(buf->avail >= 0 && "Buffer commit reduced avail to < 0.");

    buf->mark = 0;

    if(buf->avail == 0) {
        // if there's nothing available then just start at the beginning
        buf->cur = 0;
    } else {
        buf->cur += need;
    }
}


int IOBuf_send(IOBuf *buf, char *data, int len, int *out_state)
{
    int rc = 0;

    if(buf->closed) {
        rc = -1;
    } else {
        rc = buf->send(buf, data, len);
    }

    *out_state = buf->closed = rc == -1;
    return rc;
}

