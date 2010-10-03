#undef NDEBUG

#include "minunit.h"
#include <io.h>

FILE *LOG_FILE = NULL;


char *test_IOBuf_read_operations() 
{
    char *data = NULL;
    int avail = 0;

    int zero_fd = open("/dev/zero", O_RDONLY);
    IOBuf *buf = IOBuf_create(10 * 1024, zero_fd, IOBUF_FILE);
    mu_assert(buf != NULL, "Failed to allocate buf.");

    IOBuf_resize(buf, 31);
    mu_assert(buf->len == 31, "Wrong size after resize.");

    data = IOBuf_start(buf);
    avail = IOBuf_avail(buf);
    mu_assert(data != NULL, "Didn't get a data response on begin.");
    mu_assert(data == buf->buf, "Begin on fresh iobuf should be at the beginning.");
    mu_assert(avail == 0, "Should not have anything available yet.");

    data = IOBuf_read(buf, 10, &avail);
    mu_assert(data != NULL, "Should get something always.");
    mu_assert(data == IOBuf_start(buf), "First read should work from start.");
    mu_assert(data == buf->buf, "First read should be at start of internal buf.");
    mu_assert(avail == 10, "Should get 10 bytes.");

    // check compacting
    mu_assert(!IOBuf_compact_needed(buf, 10), "Should not need compacting for 10.");
    mu_assert(IOBuf_compact_needed(buf, 100), "SHOULD need compacting for 100.");

    IOBuf_read_commit(buf, 10);

    data = IOBuf_start(buf);
    avail = IOBuf_avail(buf);
    mu_assert(data != NULL, "Didn't get a data response on begin.");
    mu_assert(data != buf->buf, "Later reads should not be at the start.");
    mu_assert(avail == 21, "Should have 21 bytes available in the buf already.");

    data = IOBuf_read(buf, 10, &avail);
    mu_assert(data != NULL, "Should get something always.");
    mu_assert(avail == 10, "Should get 10 bytes.");

    IOBuf_read_commit(buf, 10);


    data = IOBuf_start(buf);
    avail = IOBuf_avail(buf);
    mu_assert(data != NULL, "Didn't get a data response on begin.");
    mu_assert(data != buf->buf, "Later reads should not be at the start.");
    mu_assert(avail == 11, "Should have 11 bytes available in the buf already.");

    // now test two reads, one that fits one that doesn't then commit
    // remember this doesn't *return* anything, just a pointer to the start
    data = IOBuf_read(buf, 5, &avail);
    mu_assert(data != NULL, "Should get something always.");
    mu_assert(avail == 5, "Should get 10 bytes.");

    // ok we didn't want 5 we want 20, so this will cause a compact
    data = IOBuf_read(buf, 20, &avail);
    mu_assert(data != NULL, "Should get something always.");
    mu_assert(avail == 20, "Should get 10 bytes.");

    IOBuf_read_commit(buf, 21);
    debug("We've got %d avail after the last read.", buf->avail);
    mu_assert(buf->avail == 10, "Should have 11 still in the queue.");

    IOBuf_destroy(buf);

    return NULL;
}

char *test_IOBuf_send_operations() 
{
    int null_fd = open("/dev/null", O_WRONLY);
    IOBuf *buf = IOBuf_create(10 * 1024, null_fd, IOBUF_FILE);
    mu_assert(buf != NULL, "Failed to allocate buf.");
    int closed = 0;

    int rc = IOBuf_send(buf, "012345789", 10, &closed);
    mu_assert(!closed, "Should not be closed.");
    mu_assert(rc == 10, "Should have sent 10 bytes.");

    fdclose(buf->fd);
    rc = IOBuf_send(buf, "012345789", 10, &closed);
    mu_assert(closed, "Should be closed.");
    mu_assert(rc == -1, "Should send nothing.");

    IOBuf_destroy(buf);

    return NULL;
}

char * all_tests() {
    mu_suite_start();

    mu_run_test(test_IOBuf_read_operations);
    mu_run_test(test_IOBuf_send_operations);

    return NULL;
}

RUN_TESTS(all_tests);

