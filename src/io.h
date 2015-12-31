#ifndef _io_h
#define _io_h

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <stdlib.h>
#include <mbedtls/x509.h>
#include <mbedtls/ssl.h>
#include "server.h"

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#include "bsd_specific.h"
#else
#include <sys/sendfile.h>
#endif

extern int MAX_SEND_BUFFER;
extern int IO_SSL_VERIFY_METHOD;

struct IOBuf;

typedef ssize_t (*io_cb)(struct IOBuf *, char *data, int len);
typedef ssize_t (*io_stream_file_cb)(struct IOBuf *, int fd, off_t len);

typedef enum IOBufType {
    IOBUF_SSL, IOBUF_SOCKET, IOBUF_FILE, IOBUF_NULL
} IOBufType;

typedef struct IOBuf {

    // len is how much space is in the buffer total
    int len;

    // avail is how much data is in the buffer to read
    int avail;

    // cur is where the next read will come from
    int cur;

    // lets us mark a point in the buffer that 
    int mark;

    int closed;
    int did_shutdown;
    io_cb recv;
    io_cb send;
    io_stream_file_cb stream_file;
    char *buf;

    int type;

    int fd;
    int use_ssl;
    int ssl_initialized;
    int handshake_performed;
    int ssl_sent_close;
    mbedtls_ssl_config sslconf;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_session ssn;
} IOBuf;

IOBuf *IOBuf_create_ssl(size_t len, int fd, int (*rng_func)(void *, unsigned char *, size_t), void *rng_ctx);
IOBuf *IOBuf_create(size_t len, int fd, IOBufType type);

int IOBuf_ssl_init(IOBuf *buf);

void IOBuf_resize(IOBuf *buf, size_t new_size);

void IOBuf_destroy(IOBuf *buf);
int IOBuf_close(IOBuf *buf);
int IOBuf_shutdown(IOBuf *buf);
int IOBuf_register_disconnect(IOBuf *buf);

char *IOBuf_read(IOBuf *buf, int need, int *out_len);
int IOBuf_read_commit(IOBuf *buf, int need);

int IOBuf_send(IOBuf *buf, char *data, int len);

int IOBuf_send_all(IOBuf *buf, char *data, int len);

char *IOBuf_read_all(IOBuf *buf, int len, int retries);

int IOBuf_stream(IOBuf *from, IOBuf *to, int total);

int IOBuf_stream_file(IOBuf *buf, int fd, off_t len);

#define IOBuf_read_some(I,A) IOBuf_read((I), (I)->len, A)

#define IOBuf_closed(I) ((I)->closed)

#define IOBuf_compact_needed(I,N) ((I)->cur + (N) > (I)->len)

#define IOBuf_remaining(I) ((I)->len - (I)->avail - (I)->cur)
#define IOBuf_read_point(I) ((I)->buf + (I)->cur + (I)->avail)
#define IOBuf_start(I) ((I)->buf + (I)->cur)

#define IOBuf_set_mark(I, N) ((I)->mark = (N))
#define IOBuf_mark(I) ((I)->mark)

#define IOBuf_size(I) ((I)->len)
#define IOBuf_avail(I) ((I)->avail)

#define IOBuf_fd(I) ((I)->fd)

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define IOBuf_sendfile bsd_sendfile
#else
#define IOBuf_sendfile sendfile
#endif

#endif
