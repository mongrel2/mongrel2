#ifndef _chunked_h
#define _chunked_h

#include "io.h"

// try to read chunk from raw buffer.
// return:
//   1: can read (and set data_pos, data_len, end_pos)
//   0: need to read more
//  -1: error
int chunked_can_read(const char *data, int size, int *data_pos, int *data_len, int *end_pos);

// block and read an entire chunk from the IOBuf. returns a pointer to the
//   start of the decoded chunk. out_len will be set to the data length.
//   read_size will be set to the actual amount of bytes consumed during the
//   read operation. the caller must call IOBuf_read_commit(iob, read_size)
//   afterwards to advance the IOBuf.
//
// this method is zero-copy thanks to the fact that chunked encoding doesn't
//   alter the enveloped data. the return value points directly to the data
//   and remains valid until IOBuf_read_commit is called.
//
char *chunked_read(IOBuf *iob, int *out_len, int *read_size);

#endif
