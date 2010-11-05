#ifndef _io_h
#define _io_h

#include <stdlib.h>
#include <ssl/ssl.h>

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__)
#include "bsd_specific.h"
#else
#include <sys/sendfile.h>
#endif

extern int MAX_SEND_BUFFER;

struct IOBuf;

typedef ssize_t (*io_cb)(struct IOBuf *, char *data, int len);
typedef ssize_t (*io_stream_file_cb)(struct IOBuf *, int fd, int len);

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
    io_cb recv;
    io_cb send;
    io_stream_file_cb stream_file;
    char *buf;

    int type;

    int fd;
    SSL *ssl;
    char *ssl_buf;
    int ssl_buf_len;
    int ssl_did_receive;
} IOBuf;

IOBuf *IOBuf_create(size_t len, int fd, IOBufType type);

void IOBuf_resize(IOBuf *buf, size_t new_size);

void IOBuf_destroy(IOBuf *buf);

char *IOBuf_read(IOBuf *buf, int need, int *out_len);
void IOBuf_read_commit(IOBuf *buf, int need);

int IOBuf_send(IOBuf *buf, char *data, int len);

int IOBuf_send_all(IOBuf *buf, char *data, int len);

char *IOBuf_read_all(IOBuf *buf, int len, int retries);

int IOBuf_stream(IOBuf *from, IOBuf *to, int total);

int IOBuf_stream_file(IOBuf *buf, int fd, int len);

#define IOBuf_read_some(I,A) IOBuf_read((I), (I)->len, A)

#define IOBuf_closed(I) ((I)->closed)

#define IOBuf_compact_needed(I,N) ((I)->cur + (N) > (I)->len)

#define IOBuf_remaining(I) ((I)->len - (I)->avail - (I)->cur)
#define IOBuf_read_point(I) ((I)->buf + (I)->cur + (I)->avail)
#define IOBuf_start(I) ((I)->buf + (I)->cur)

#define IOBuf_set_mark(I, N) ((I)->mark = (N))
#define IOBuf_mark(I) ((I)->mark)

#define IOBuf_avail(I) ((I)->avail)

#define IOBuf_fd(I) ((I)->fd)

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__)
#define Dir_send bsd_sendfile
#else
#define Dir_send sendfile
#endif

#endif
