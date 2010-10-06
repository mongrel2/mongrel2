
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

    buf->buf = h_malloc(len + 1);
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
    if(buf) {
        fdclose(buf->fd);
        if(buf->ssl) ssl_free(buf->ssl);
        h_free(buf);
    }
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

/**
 * Does a non-blocking read and either reads in the amount you requested
 * with "need" or less so you can try again.  It sets out_len to what is
 * available (<= need) so you can decide after that.  You can keep attempting
 * to read more and more (up to buf->len) and you'll get the same start point
 * each time.
 *
 * Once you're done with the data you've been trying to read, you use IOBuf_read_commit
 * to commit the "need" amount and then start doing new reads.
 *
 * Internally this works like a "half open ring buffer" and it tries to greedily
 * read as much as possible without blocking or copying.
 *
 * To just get as much as possible, use the IOBuf_read_some macro.
 */
char *IOBuf_read(IOBuf *buf, int need, int *out_len)
{
    debug("Asked to read %d from buffer with %d avail and %d len", 
            need, buf->avail, buf->len);

    int rc = 0;
    assert(buf->cur + buf->avail <= buf->len && 
            "Buffer math off, cur+avail can't be more than > len.");
    assert(buf->cur >= 0 && "Buffer cur can't be < 0");
    assert(buf->avail >= 0 && "Buffer avail can't be < 0");
    assert(need <= buf->len && "Request for more than possible in the buffer.");

    if(IOBuf_closed(buf) && buf->avail > 0) {
        *out_len = buf->avail;
    } else if(buf->avail < need) {
        debug("need to fill the buffer with more, attempt a read");
        if(buf->cur > 0 && IOBuf_compact_needed(buf, need)) {
            IOBuf_compact(buf);
        }

        debug("Attempting to read as much as possible: remain: %d, avail: %d", 
                IOBuf_remaining(buf), buf->avail);
        rc = buf->recv(buf, IOBuf_read_point(buf), IOBuf_remaining(buf));

        // TODO: determine if this should be <=
        if(rc <= 0) {
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
    } else if(buf->avail >= need) {
        debug("We have enough to satisfy the request so return it.");
        *out_len = need;
    } else {
        assert(0 && "Invalid branch processing buffer, Tell Zed.");
    }

    debug("Out length: %d", *out_len);

    // now everything should be good to go from cur point
    return IOBuf_start(buf);
}

/**
 * Commits the amount given by need to the buffer, moving the internal
 * counters around so that the next read starts after the need point.
 * You use this after you're done working with the data and want to
 * do the next read.
 */
void IOBuf_read_commit(IOBuf *buf, int need)
{
    debug("COMMIT: %d", need);
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

/**
 * Wraps the usual send, so not much to it other than it'll avoid doing
 * any calls if the socket is already closed.
 */
int IOBuf_send(IOBuf *buf, char *data, int len)
{
    int rc = 0;

    assert(!buf->closed && "CLOSED SEND!");

    rc = buf->send(buf, data, len);

    if(rc < 0) buf->closed = 1;

    return rc;
}

/**
 * Reads the entire amount requested into the IOBuf (as long as there's
 * space to hold it) and then commits that read in one shot.
 */
char *IOBuf_read_all(IOBuf *buf, int len, int retries)
{
    debug("Asked to read ALL %d from buffer with %d avail and %d len", 
            len, buf->avail, buf->len);

    int nread = 0;
    assert(len <= buf->len && "Cannot read more than the buffer length.");

    char *data = IOBuf_read(buf, len, &nread);
    check(!IOBuf_closed(buf), "Closed when attempting to read from buffer.");

    while(nread < len && retries > 0) {
        data = IOBuf_read(buf, len, &nread);
        check(data, "Read error from socket.");
        assert(nread <= len && "Invalid nread size (too much) on IOBuf read.");

        if(nread == len) {
            break;
        } else {
            retries--;
            fdwait(buf->fd, 'r');
        }
    }

    check(retries > 0, "Too many retries (%d) while reading.", retries);
    IOBuf_read_commit(buf, len);
    return data;

error:
    return NULL;
}


/**
 * Streams data out of the from IOBuf and into the to IOBuf
 * until it's moved total bytes between them.
 */
int IOBuf_stream(IOBuf *from, IOBuf *to, int total)
{
    int need = 0;
    int remain = total;
    int avail = 0;
    int rc = 0;
    char *data = NULL;

    debug("SENDING: %d total", total);

    if(from->len > to->len) IOBuf_resize(to, from->len);

    while(remain > 0) {
        need = remain <= from->len ? remain : from->len;
        debug("REMAIN: %d", remain);

        data = IOBuf_read(from, need, &avail);
        check_debug(avail > 0, "Nothing in read buffer.");

        rc = IOBuf_send_all(to, IOBuf_start(from), avail);
        check_debug(rc == avail, "Failed to send all of the data: %d of %d", rc, avail)

        // commit whatever we just did
        IOBuf_read_commit(from, rc);

        // reduce it by the given amount
        remain -= rc;
    }

    assert(remain == 0 && "Buffer math is wrong.");

    return total - remain;

error:
    return -1;
}



int IOBuf_send_all(IOBuf *buf, char *data, int len)
{
    int rc = 0;
    int total = len;
    do {
        rc = IOBuf_send(buf, data, total);
        check(rc > 0, "Write error when sending all.");
        total -= rc;
    } while(total > 0);

    assert(total == 0 && "Math error sending all from buffer.");

    return len;

error:
    return -1;
}
