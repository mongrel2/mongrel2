#include "chunked.h"

#include <string.h>
#include "connection.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

static int strfind(const char *haystack, int size, const char *needle, int offset)
{
    int nsize;
    int n;

    nsize = strlen(needle);
    if(nsize <= 0)
        return 0;

    for(n = offset; n <= size - nsize; ++n) {
        if(strncmp(haystack + n, needle, nsize) == 0)
            return n;
    }

    return -1;
}

int chunked_can_read(const char *data, int size, int *data_pos, int *data_len, int *end_pos)
{
    int at;
    int end;
    char sizestr[9];
    int chunk_size;

    at = strfind(data, MIN(size, 10), "\r\n", 0);
    if(at == -1) {
        if(size >= 10) {
            // error if we don't find size quickly
            return -1;
        } else {
            return 0;
        }
    }

    assert(at <= 8);

    strncpy(sizestr, data, at);
    sizestr[at] = 0;
    chunk_size = strtol(sizestr, NULL, 16);
    if(chunk_size > MAX_CHUNK_SIZE)
        return -1;

    if(chunk_size > 0) {
        if(size < at + 2 + chunk_size + 2)
            return 0;
        if(data[at + 2 + chunk_size] != '\r')
            return -1;
        if(data[at + 2 + chunk_size + 1] != '\n')
            return -1;

        *data_pos = at + 2;
        *data_len = chunk_size;
        *end_pos = at + 2 + chunk_size + 2;
        return 1;
    } else {
        // if chunk size is 0 then there could be trailing headers. look for
        //   double newline
        end = strfind(data, size, "\r\n\r\n", at);
        if(end == -1) {
            if(size > MAX_CONTENT_LENGTH) {
                // trailing section too large
                return -1;
            } else {
                return 0;
            }
        }

        // note: if we ever support trailing headers,
        //   the length is end + 2 - (at + 2)

        *data_pos = at + 2;
        *data_len = 0;
        *end_pos = end + 4;
        return 1;
    }
}

char *chunked_read(IOBuf *iob, int *out_len, int *read_size)
{
    char *data = NULL;
    int avail = 0;
    int rc;
    int data_pos;
    int data_len;
    int end_pos;

    data = IOBuf_read(iob, IOBuf_avail(iob), &avail);
    while(1) {
        rc = chunked_can_read(data, avail, &data_pos, &data_len, &end_pos);
        if(rc == 1) {
            break;
        } else if(rc == 0 && IOBuf_size(iob) - IOBuf_avail(iob) > 0) {
            // we need data. ensure the socket is still open
            check(!IOBuf_closed(iob), "Closed while reading from IOBuf.");
            // wait for new data
            data = IOBuf_read(iob, avail + 1, &avail);
            // examine all that we can
            avail = IOBuf_avail(iob);
        } else {
            return NULL;
        }
    }

    *out_len = data_len;
    *read_size = end_pos;
    return data + data_pos;

error:
    return NULL;
}
