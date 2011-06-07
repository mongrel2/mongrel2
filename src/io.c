#define _XOPEN_SOURCE 500
#define _FILE_OFFSET_BITS 64

#include "io.h"
#include "register.h"
#include "mem/halloc.h"
#include "dbg.h"
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <polarssl/havege.h>
#include <polarssl/ssl.h>
#include <task/task.h>

static ssize_t null_send(IOBuf *iob, char *buffer, int len)
{
    return len;
}

static ssize_t null_recv(IOBuf *iob, char *buffer, int len)
{
    return len;
}

static ssize_t null_stream_file(IOBuf *iob, int fd, off_t len)
{
    return len;
}

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

static ssize_t plain_stream_file(IOBuf *iob, int fd, off_t len)
{
    ssize_t sent = 0;
    off_t total = 0;
    off_t offset = 0;
    off_t block_size = MAX_SEND_BUFFER;
    int conn_fd = IOBuf_fd(iob);

    for(total = 0; fdwait(conn_fd, 'w') == 0 && total < len; total += sent) {

        block_size = (len - total) > block_size ? block_size : (len - total);
        sent = IOBuf_sendfile(conn_fd, fd, &offset, block_size);

        check(Register_write(iob->fd, sent) != -1, "Socket seems to be closed.");

        check_debug(sent > 0, "Client closed probably during sendfile on socket: %d from "
                    "file %d", conn_fd, fd);
    }
    
    check(total <= len,
            "Wrote way too much, wrote %d but size was %zd",
            (int)total, len);

    check(total == len,
            "Sent other than expected, sent: %d, but expected: %zd", 
            (int)total, len);

    return total;

error:
    return -1;
}

static int ssl_fdsend_wrapper(void *p_iob, unsigned char *ubuffer, int len)
{
    IOBuf *iob = (IOBuf *) p_iob;
    return fdsend(iob->fd, (char *) ubuffer, len);
}

static int ssl_fdrecv_wrapper(void *p_iob, unsigned char *ubuffer, int len)
{
    IOBuf *iob = (IOBuf *) p_iob;
    return fdrecv1(iob->fd, (char *) ubuffer, len);
}

static int ssl_do_handshake(IOBuf *iob)
{
    int rcode;
    check(!iob->handshake_performed, "ssl_do_handshake called unnecessarily");
    while((rcode = ssl_handshake(&iob->ssl)) != 0) {
        check(rcode == POLARSSL_ERR_NET_TRY_AGAIN,
              "handshake failed with error code %d", rcode);
    }
    iob->handshake_performed = 1;
    return 0;
error:
    return -1;
}

static ssize_t ssl_send(IOBuf *iob, char *buffer, int len)
{
    check(iob->use_ssl, "IOBuf not set up to use ssl");
    if(!iob->handshake_performed) {
        int rcode = ssl_do_handshake(iob);
        check(rcode == 0, "handshake failed");
    }
    return ssl_write(&iob->ssl, (const unsigned char*) buffer, len);
error:
    return -1;
}

static ssize_t ssl_recv(IOBuf *iob, char *buffer, int len)
{
    check(iob->use_ssl, "IOBuf not set up to use ssl");
    if(!iob->handshake_performed) {
        int rcode = ssl_do_handshake(iob);
        check(rcode == 0, "handshake failed");
    }
    return ssl_read(&iob->ssl, (unsigned char*) buffer, len);
error:
    return -1;
}

static ssize_t ssl_stream_file(IOBuf *iob, int fd, off_t len)
{
    ssize_t sent = 0;
    off_t total = 0;
    ssize_t amt = 0;
    ssize_t tosend = 0;
    int conn_fd = IOBuf_fd(iob);
    char buff[1024];

    for(total = 0; fdwait(conn_fd, 'w') == 0 && total < len; total += tosend) {
        tosend = pread(fd, buff, sizeof(buff), total);
        check_debug(tosend > 0, "Came up short in reading file %d\n", fd);

        // We do this in case the file somehow lengthened on us.  In general,
        // it shouldn't happen.
        if(tosend + total > len)
            tosend = len - total;

        sent = 0;
        while(sent < tosend) {
            amt = ssl_send(iob, buff, tosend);
            check_debug(amt > 0, "ssl_send failed in ssl_stream_file with "
                        "return code %zd", amt);
            sent += amt;
        }

        check(Register_write(iob->fd, sent) != -1, "Failed to record write, must have died.");
    }
    
    check(total <= len,
            "Wrote way too much, wrote %d but size was %zd",
            (int)total, len);

    check(total == len,
            "Sent other than expected, sent: %d, but expected: %zd", 
            (int)total, len);

    return total;

error:
    return -1;
}

void ssl_debug(void *p, int level, const char *msg) {
    if(level < 2)
        debug("polarssl: %s", msg);
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
        buf->use_ssl = 1;
        buf->handshake_performed = 0;
        ssl_init(&buf->ssl);
        ssl_set_endpoint(&buf->ssl, SSL_IS_SERVER);
        ssl_set_authmode(&buf->ssl, SSL_VERIFY_NONE);
        havege_init(&buf->hs);
        ssl_set_rng(&buf->ssl, havege_rand, &buf->hs);
        ssl_set_dbg(&buf->ssl, ssl_debug, NULL);
        ssl_set_bio(&buf->ssl, ssl_fdrecv_wrapper, buf, 
                    ssl_fdsend_wrapper, buf);
        ssl_set_session(&buf->ssl, 1, 0, &buf->ssn);
        memset(&buf->ssn, 0, sizeof(buf->ssn));

        buf->send = ssl_send;
        buf->recv = ssl_recv;
        buf->stream_file = ssl_stream_file;
    } else if(type == IOBUF_NULL) {
        buf->send = null_send;
        buf->recv = null_recv;
        buf->stream_file = null_stream_file;
    } else if(type == IOBUF_FILE) {
        buf->send = file_send;
        buf->recv = file_recv;
        buf->stream_file = plain_stream_file;
    } else if(type == IOBUF_SOCKET) {
        buf->send = plaintext_send;
        buf->recv = plaintext_recv;
        buf->stream_file = plain_stream_file;
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
        if(buf->use_ssl) {
            ssl_close_notify(&buf->ssl);
            ssl_free(&buf->ssl);
        }
        
        fdclose(buf->fd);
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
    int rc = 0;
    assert(buf->cur + buf->avail <= buf->len && 
            "Buffer math off, cur+avail can't be more than > len.");
    assert(buf->cur >= 0 && "Buffer cur can't be < 0");
    assert(buf->avail >= 0 && "Buffer avail can't be < 0");
    assert(need <= buf->len && "Request for more than possible in the buffer.");

    if(IOBuf_closed(buf)) {
        if(buf->avail > 0) {
            *out_len = buf->avail;
        } else {
            *out_len = 0;
            return NULL;
        }
    } else if(buf->avail < need) {
        if(buf->cur > 0 && IOBuf_compact_needed(buf, need)) {
            IOBuf_compact(buf);
        }
        rc = buf->recv(buf, IOBuf_read_point(buf), IOBuf_remaining(buf));

        if(rc <= 0) {
            debug("Socket was closed, will return only what's available: %d", buf->avail);
            buf->closed = 1;
        } else {
            buf->avail = buf->avail + rc;
        }

        if(buf->avail < need) {
            // less than expected
            *out_len = buf->avail;
        } else {
            // more than expected
            *out_len = need;
        }
    } else if(buf->avail >= need) {
        *out_len = need;
    } else {
        sentinel("Invalid branch processing buffer, Tell Zed.");
    }

    return IOBuf_start(buf);

error:
    return NULL;
}

/**
 * Commits the amount given by need to the buffer, moving the internal
 * counters around so that the next read starts after the need point.
 * You use this after you're done working with the data and want to
 * do the next read.
 */
int IOBuf_read_commit(IOBuf *buf, int need)
{
    buf->avail -= need;
    assert(buf->avail >= 0 && "Buffer commit reduced avail to < 0.");
    
    check(Register_read(buf->fd, need) != -1, "Failed to record read, must have died.");

    buf->mark = 0;

    if(buf->avail == 0) {
        // if there's nothing available then just start at the beginning
        buf->cur = 0;
    } else {
        buf->cur += need;
    }

    return 0;
error:
    return -1;
}

/**
 * Wraps the usual send, so not much to it other than it'll avoid doing
 * any calls if the socket is already closed.
 */
int IOBuf_send(IOBuf *buf, char *data, int len)
{
    int rc = 0;

    rc = buf->send(buf, data, len);

    if(rc >= 0) {
        check(Register_write(buf->fd, rc) != -1, "Failed to record write, must have died.");
    } else {
        buf->closed = 1;
    }

    return rc;

error:
    return -1;
}

/**
 * Reads the entire amount requested into the IOBuf (as long as there's
 * space to hold it) and then commits that read in one shot.
 */
char *IOBuf_read_all(IOBuf *buf, int len, int retries)
{
    int nread = 0;
    int attempts = 0;

    check_debug(!IOBuf_closed(buf), "Closed when attempting to read from buffer.");

    if(len > buf->len) {
        // temporarily grow the buffer
        IOBuf_resize(buf, len);
    }

    char *data = IOBuf_read(buf, len, &nread);

    debug("INITIAL READ: len: %d, nread: %d", len, nread);
    for(attempts = 0; nread < len; attempts++) {
        data = IOBuf_read(buf, len, &nread);

        check_debug(data, "Read error from socket.");
        assert(nread <= len && "Invalid nread size (too much) on IOBuf read.");

        if(nread == len) {
            break;
        } else {
            fdwait(buf->fd, 'r');
        }
    }
   
    debug("ATTEMPTS: %d, RETRIES: %d", attempts, retries);

    if(attempts > retries) {
        log_err("Read of %d length attempted %d times which is over %d retry limit..", len, attempts, retries);
    }

    check(IOBuf_read_commit(buf, len) != -1, "Final commit failed of read_all.");
    return data;

error:
    buf->closed = 1;
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

    if(from->len > to->len) IOBuf_resize(to, from->len);

    while(remain > 0) {
        need = remain <= from->len ? remain : from->len;

        data = IOBuf_read(from, need, &avail);
        check_debug(avail > 0, "Nothing in read buffer.");

        rc = IOBuf_send_all(to, IOBuf_start(from), avail);
        check_debug(rc == avail, "Failed to send all of the data: %d of %d", rc, avail)

        // commit whatever we just did
        check(IOBuf_read_commit(from, rc) != -1, "Final commit failed during streaming.");

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

int IOBuf_stream_file(IOBuf *buf, int fd, off_t len)
{
    int rc = 0;

    // We depend on the stream_file callback to call Register_write.
    // Doing it here would make the connection look inactive for long periods
    // if we are streaming a large file.
    rc = buf->stream_file(buf, fd, len);

    if(rc < 0) buf->closed = 1;

    return rc;
}



void debug_dump(void *addr, int len)
{
#ifdef NDEBUG
  return;
#endif

  char tohex[] = "0123456789ABCDEF";
  int i = 0;
  unsigned char *pc = addr;

  char buf0[32] = {0};                // offset
  char buf1[64] = {0};                // hex
  char buf2[64] = {0};                // literal

  char *pc1 = NULL;
  char *pc2 = NULL;

  

  while(--len >= 0) {

    if(i % 16 == 0) {
      sprintf(buf0, "%08x", i);
      buf1[0] = 0;
      buf2[0] = 0;
      pc1 = buf1;
      pc2 = buf2;
    }

    *pc1++ = tohex[*pc >> 4];
    *pc1++ = tohex[*pc & 15];
    *pc1++ = ' ';

    if(*pc >= 32 && *pc < 127) {
      *pc2++ = *pc;
    } else {
      *pc2++ = '.';
    }

    i++;
    pc++;

    if(i % 16 == 0) {
      *pc1 = 0;
      *pc2 = 0;
      debug("%s:   %s  %s", buf0, buf1, buf2);
    }

  }

  if(i % 16 != 0) {
    while(i % 16 != 0) {
      *pc1++ = ' ';
      *pc1++ = ' ';
      *pc1++ = ' ';
      *pc2++ = ' ';
      i++;
    }

    *pc1 = 0;
    *pc2 = 0;
    debug("%s:   %s  %s", buf0, buf1, buf2);
  }
}
