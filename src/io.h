#ifndef _io_h
#define _io_h

#include <stdlib.h>
#include <ssl/ssl.h>

struct IOBuf;

typedef ssize_t (*io_cb)(struct IOBuf *, char *data, int len);

typedef enum IOBufType {
    IOBUF_SSL, IOBUF_SOCKET, IOBUF_FILE
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
    char *buf;

    int type;

    int fd;
    SSL *ssl;
    char *ssl_buf;
    int ssl_buf_len;
} IOBuf;

IOBuf *IOBuf_create(size_t len, int fd, IOBufType type);

void IOBuf_resize(IOBuf *buf, size_t new_size);

void IOBuf_destroy(IOBuf *buf);

char *IOBuf_read(IOBuf *buf, int need, int *out_len);
void IOBuf_read_commit(IOBuf *buf, int need);

int IOBuf_send(IOBuf *buf, char *data, int len, int *out_state);


#define IOBuf_closed(I) ((I)->closed)

#define IOBuf_compact_needed(I,N) ((I)->cur + (N) > (I)->len)

#define IOBuf_remaining(I) ((I)->len - (I)->avail)
#define IOBuf_read_point(I) ((I)->buf + (I)->cur + (I)->avail)
#define IOBuf_start(I) ((I)->buf + (I)->cur)

#define IOBuf_set_mark(I, N) ((I)->mark = (N))
#define IOBuf_mark(I) ((I)->mark)

#define IOBuf_avail(I) ((I)->avail)

#define IOBuf_fd(I) ((I)->fd)

#endif
