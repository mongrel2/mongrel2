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

#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "io.h"
#include "register.h"
#include "mem/halloc.h"
#include "dbg.h"
#include "mbedtls/ssl.h"
#include "task/task.h"
#include "adt/darray.h"

int IO_SSL_VERIFY_METHOD = MBEDTLS_SSL_VERIFY_NONE;

static ssize_t null_send(IOBuf *iob, char *buffer, int len)
{
    (void)iob;
    (void)buffer;

    return len;
}

static ssize_t null_recv(IOBuf *iob, char *buffer, int len)
{
    (void)iob;
    (void)buffer;

    return len;
}

static ssize_t null_stream_file(IOBuf *iob, int fd, off_t len)
{
    (void)iob;
    (void)fd;

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
    off_t sent;
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

static int ssl_fdsend_wrapper(void *p_iob, const unsigned char *ubuffer, size_t len)
{
    IOBuf *iob = (IOBuf *) p_iob;
    return fdsend(iob->fd, (char *) ubuffer, len);
}

static int ssl_fdrecv_wrapper(void *p_iob, unsigned char *ubuffer, size_t len, uint32_t timeout)
{
    (void)timeout; // ignore timeout

    IOBuf *iob = (IOBuf *) p_iob;
    return fdrecv1(iob->fd, (char *) ubuffer, len);
}

static int ssl_do_handshake(IOBuf *iob)
{
    int rcode;
    check(!iob->handshake_performed, "ssl_do_handshake called unnecessarily");
    while((rcode = mbedtls_ssl_handshake(&iob->ssl)) != 0) {

        check(rcode == MBEDTLS_ERR_SSL_WANT_READ
                || rcode == MBEDTLS_ERR_SSL_WANT_WRITE, "Handshake failed with error code: %d", rcode);
    }
    iob->handshake_performed = 1;
    return 0;
error:
    return -1;
}

static ssize_t ssl_send(IOBuf *iob, char *buffer, int len)
{
    int sent;
    int total = 0;

    check(iob->use_ssl, "IOBuf not set up to use ssl");

    if(!iob->handshake_performed) {
        int rcode = ssl_do_handshake(iob);
        check(rcode == 0, "SSL handshake failed: %d", rcode);
    }

    for(; len > 0; buffer += sent, len -= sent, total += sent) {
        sent = mbedtls_ssl_write(&iob->ssl, (const unsigned char*) buffer, len);

        check(sent > 0, "Error sending SSL data.");
        check(sent <= len, "Buffer overflow. Too much data sent by ssl_write");

        // make sure we don't hog the process trying to stream out
        if(sent < len) {
            taskyield();
        }
    };

    return total;

error:
    return -1;
}

static ssize_t ssl_recv(IOBuf *iob, char *buffer, int len)
{
    int rc;
    check(iob->use_ssl, "IOBuf not set up to use ssl");

    if(!iob->handshake_performed) {
        int rcode = ssl_do_handshake(iob);
        check(rcode == 0, "SSL handshake failed: %d", rcode);
    }

    rc = mbedtls_ssl_read(&iob->ssl, (unsigned char*) buffer, len);

    // we count EOF as error (but this may be too common to log a message)
    if(rc == 0) {
        goto error;
    }

    // we count close notify as EOF
    if(rc == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        return 0;
    }

    // we either return non-zero length or an error here
    return rc;
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

void ssl_debug(void *p, int level, const char *fname, int line, const char *msg)
{
    (void)p;
    (void)fname;
    (void)line;
    if (msg) {}

    if(level < 2) {
        debug("mbedtls: %s", msg);
    }
}

// -- quick hack taken from the polar ssl server code to see how it will work
//
/*
 * These session callbacks use a simple chained list
 * to store and retrieve the session information.
 */
static darray_t *SSL_SESSION_CACHE = NULL;
const int SSL_INITIAL_CACHE_SIZE = 300;
const int SSL_CACHE_SIZE_LIMIT = 1000;
const int SSL_CACHE_LIMIT_REMOVE_COUNT = 100;

static inline int setup_ssl_session_cache()
{
    if(SSL_SESSION_CACHE == NULL) {
        SSL_SESSION_CACHE = darray_create(SSL_INITIAL_CACHE_SIZE, sizeof(mbedtls_ssl_session));
        check_mem(SSL_SESSION_CACHE);
    }
    return 0;
error:
    return -1;
}


static int simple_get_cache( void *p, mbedtls_ssl_session *ssn )
{
    (void)p;
    int i = 0;

    check(setup_ssl_session_cache() == 0, "Failed to initialize SSL session cache.");

    mbedtls_ssl_session *cur = NULL;

    for(i = 0; i < darray_end(SSL_SESSION_CACHE); i++) {
        cur = darray_get(SSL_SESSION_CACHE, i);

        if( ssn->ciphersuite != cur->ciphersuite ||
            ssn->id_len != cur->id_len )
        {
            continue;
        }

        if( memcmp( ssn->id, cur->id, cur->id_len ) != 0 ) {
            continue;
        }

        darray_move_to_end(SSL_SESSION_CACHE, i);

        // TODO: odd, why 48? this is from polarssl
        memcpy( ssn->master, cur->master, 48 );

        mbedtls_x509_crt* _x509P  = cur->peer_cert;
        if (_x509P == NULL) {
            debug("failed to find peer_cert in handshake during get");
            return 0;
        }
        ssn->peer_cert=_x509P;

        return 0;
    }

error: // fallthrough
    return 1;
}

static int simple_set_cache( void *p_ssl, const mbedtls_ssl_session *ssn )
{
    mbedtls_ssl_context *ssl = (mbedtls_ssl_context *) p_ssl;
    int i = 0;
    mbedtls_ssl_session *cur = NULL;
    int make_new = 1;
    check(setup_ssl_session_cache() == 0, "Failed to initialize SSL session cache.");

    for(i = 0; i < darray_end(SSL_SESSION_CACHE); i++) {
        cur = darray_get(SSL_SESSION_CACHE, i);

        if( memcmp( ssn->id, cur->id, cur->id_len ) == 0 ) {
            make_new = 0;
            break; /* client reconnected */
        }
    }

    if(make_new) {
        if (darray_end(SSL_SESSION_CACHE) >= SSL_CACHE_SIZE_LIMIT) {
            darray_remove_and_resize(SSL_SESSION_CACHE, 0, SSL_CACHE_LIMIT_REMOVE_COUNT);
        }

        cur = (mbedtls_ssl_session *) darray_new(SSL_SESSION_CACHE);
        check_mem(cur);
        darray_push(SSL_SESSION_CACHE, cur);
    }
    else {
        darray_move_to_end(SSL_SESSION_CACHE, i);
    }

    *cur = *ssn;

    const mbedtls_x509_crt* _x509P  = mbedtls_ssl_get_peer_cert( ssl );
    if (_x509P == NULL) {
        debug("failed to find peer_cert in handshake");
        return 0;
    }

    int rc = 0;
    if ((rc = mbedtls_x509_crt_parse( cur->peer_cert,  _x509P->raw.p, _x509P->raw.len)) != 0) {
        debug("failed to set peer_cert during handshake:rc:%d:", rc);
    }

    return 0;
error:
    return 1;
}



static inline int iobuf_ssl_setup(int (*rng_func)(void *, unsigned char *, size_t), void *rng_ctx, IOBuf *buf)
{
    int rc = 0;

    buf->use_ssl = 1;
    buf->handshake_performed = 0;

    memset(&buf->sslconf, 0, sizeof(mbedtls_ssl_config));
    mbedtls_ssl_config_init(&buf->sslconf);

    rc = mbedtls_ssl_config_defaults(&buf->sslconf, MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM, 0);
    check(rc == 0, "Failed to initialize SSL config structure.");

    mbedtls_ssl_conf_authmode(&buf->sslconf, IO_SSL_VERIFY_METHOD);

    mbedtls_ssl_conf_rng(&buf->sslconf, rng_func, rng_ctx);

#ifndef DEBUG
    mbedtls_ssl_conf_dbg(&buf->sslconf, ssl_debug, NULL);
#endif

    mbedtls_ssl_conf_session_cache(&buf->sslconf, &buf->ssl, simple_get_cache, simple_set_cache);

    // zero out the ssl struct here just to be safe, even though
    //   initialization happens in IOBuf_ssl_init
    memset(&buf->ssl, 0, sizeof(mbedtls_ssl_context));

    return 0;
error:
    return -1;
}

static IOBuf *IOBuf_create_internal(size_t len, int fd, IOBufType type,
        int (*rng_func)(void *, unsigned char *, size_t), void *rng_ctx)
{
    IOBuf *buf = malloc(sizeof(IOBuf));
    check_mem(buf);

    buf->len = len;
    buf->avail = 0;
    buf->cur = 0;
    buf->closed = 0;
    buf->did_shutdown = 0;
    buf->buf = malloc(len + 1);
    check_mem(buf->buf);
    buf->type = type;
    buf->fd = fd;
    buf->use_ssl = 0;
    buf->ssl_sent_close = 0;

    if(type == IOBUF_SSL) {
        check(rng_func != NULL, "IOBUF_SSL requires non-null server");
        check(iobuf_ssl_setup(rng_func, rng_ctx, buf) != -1, "Failed to setup SSL.");
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

IOBuf *IOBuf_create(size_t len, int fd, IOBufType type)
{
    check(type != IOBUF_SSL, "Use IOBuf_create_ssl for ssl IOBuffers")
    return IOBuf_create_internal(len, fd, type, NULL, NULL);
error:
    return NULL;
}

IOBuf *IOBuf_create_ssl(size_t len, int fd, int (*rng_func)(void *, unsigned char *, size_t), void *rng_ctx)
{
    return IOBuf_create_internal(len,fd,IOBUF_SSL,rng_func,rng_ctx);
}

int IOBuf_ssl_init(IOBuf *buf)
{
    int rc = 0;

    mbedtls_ssl_init(&buf->ssl);

    rc = mbedtls_ssl_setup(&buf->ssl, &buf->sslconf);
    check(rc == 0, "Failed to initialize SSL structure.");

    mbedtls_ssl_set_bio(&buf->ssl, buf, ssl_fdsend_wrapper,
                NULL, ssl_fdrecv_wrapper);

    memset(&buf->ssn, 0, sizeof(buf->ssn));
    mbedtls_ssl_set_session(&buf->ssl, &buf->ssn);

    buf->ssl_initialized = 1;
    return 0;
error:
    return -1;
}

int IOBuf_close(IOBuf *buf)
{
    int rc = 0;

    if(buf) {
        if(!buf->did_shutdown) {
            rc = IOBuf_shutdown(buf);
        }

        fdclose(buf->fd);
        buf->fd=-1;
    }

    return rc;
}

int IOBuf_shutdown(IOBuf *buf)
{
    int rc = -1;

    if(!buf || buf->fd < 0) {
        goto error;
    }

    if(buf->use_ssl && buf->handshake_performed && !buf->ssl_sent_close) {
        rc = mbedtls_ssl_close_notify(&buf->ssl);
        check(rc == 0, "ssl_close_notify failed with error code: %d", rc);

        buf->ssl_sent_close = 1;
    }

    // ignore return value, since peer may have closed
    shutdown(buf->fd, SHUT_RDWR);

    buf->did_shutdown = 1;

error:
    return rc;
}

void IOBuf_destroy(IOBuf *buf)
{
    if(buf) {
        if(buf->fd >= 0) {
            IOBuf_close(buf); // ignore return
        }

        if(buf->use_ssl) {
            if(buf->ssl_initialized) {
                mbedtls_ssl_free(&buf->ssl);
            }
            mbedtls_ssl_config_free(&buf->sslconf);
        }

        if(buf->buf) free(buf->buf);
        free(buf);
    }
}

void IOBuf_resize(IOBuf *buf, size_t new_size)
{
    buf->buf = realloc(buf->buf, new_size);
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
            check(!IOBuf_closed(buf), "Socket closed during IOBuf_read_all.");
            fdwait(buf->fd, 'r');
        }
    }
   
    debug("ATTEMPTS: %d, RETRIES: %d", attempts, retries);

    if(attempts > retries) {
        log_warn("Read of %d length attempted %d times which is over %d retry limit..", len, attempts, retries);
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

    if(from->len > to->len) IOBuf_resize(to, from->len);

    while(remain > 0) {
        need = remain <= from->len ? remain : from->len;

        IOBuf_read(from, need, &avail);
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

int IOBuf_register_disconnect(IOBuf *buf)
{
    int rc=0;
    if(IOBuf_fd(buf)>0) {
        rc= Register_disconnect(IOBuf_fd(buf));
        return rc;
    }
    return 0;
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


