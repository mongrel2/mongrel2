
#include "minunit.h"
#include <io.h>
#include <connection.h>
#include <register.h>
#include <assert.h>
#include <mem/halloc.h>

FILE *LOG_FILE = NULL;

Connection *fake_conn(const char *file, int mode) {
    Connection *conn = h_calloc(sizeof(Connection), 1);
    assert(conn && "Failed to create connection.");

    int fd = open(file, mode);
    assert(fd != -1 && "Failed to open file.");

    conn->iob = IOBuf_create(10 * 1024, fd, IOBUF_FILE);
    assert(conn->iob && "Failed to create iobuffer.");
    conn->type = CONN_TYPE_HTTP;

    Register_connect(fd, conn);

    return conn;
}

void fake_conn_close(Connection *conn)
{
    assert(conn && conn->iob && "Invalid connection.");
    Register_disconnect(conn->iob->fd);
    Connection_destroy(conn);
}

char *test_IOBuf_read_operations() 
{
    char *data = NULL;
    int avail = 0;
    Connection *conn = fake_conn("/dev/zero", O_RDONLY);
    mu_assert(conn != NULL, "Failed to make fake /dev/zero connection.");
    IOBuf *buf = conn->iob;

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

    mu_assert(IOBuf_read_commit(buf, 10) != -1, "Failed to commit.");

    data = IOBuf_start(buf);
    avail = IOBuf_avail(buf);
    mu_assert(data != NULL, "Didn't get a data response on begin.");
    mu_assert(data != buf->buf, "Later reads should not be at the start.");
    mu_assert(avail == 21, "Should have 21 bytes available in the buf already.");

    data = IOBuf_read(buf, 10, &avail);
    mu_assert(data != NULL, "Should get something always.");
    mu_assert(avail == 10, "Should get 10 bytes.");

    mu_assert(IOBuf_read_commit(buf, 10) != -1, "Finaly commit failed.");


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

    mu_assert(IOBuf_read_commit(buf, 21) != -1, "Final commit failed.");
    debug("We've got %d avail after the last read.", buf->avail);
    mu_assert(buf->avail == 10, "Should have 11 still in the queue.");

    data = IOBuf_read_all(buf, 30, 1);
    mu_assert(data != NULL, "Failed to read all.");
    mu_assert(buf->avail == 1, "Should have 1 in the queue.");

    data = IOBuf_read_some(buf, &avail);
    mu_assert(data != NULL, "Failed to read some.");
    mu_assert(buf->avail == 31, "Should be full.");
    mu_assert(avail == 31, "And we should get the full amount.");

    fake_conn_close(conn);
    return NULL;
}

char *test_IOBuf_send_operations() 
{
    Connection *conn = fake_conn("/dev/null", O_WRONLY);
    mu_assert(conn != NULL, "Failed to allocate buf.");
    IOBuf *buf = conn->iob;
    mu_assert(Register_fd_exists(IOBuf_fd(buf)) != NULL, "Damn fd isn't registered.");

    int rc = IOBuf_send(buf, "012345789", 10);
    mu_assert(!IOBuf_closed(buf), "Should not be closed.");
    mu_assert(rc == 10, "Should have sent 10 bytes.");

    fdclose(IOBuf_fd(buf));
    rc = IOBuf_send(buf, "012345789", 10);
    mu_assert(IOBuf_closed(buf), "Should be closed.");
    mu_assert(rc == -1, "Should send nothing.");

    fake_conn_close(conn);

    return NULL;
}


char *test_IOBuf_streaming()
{
    // test streaming from /dev/zero to /dev/null
    Connection *zero = fake_conn("/dev/zero", O_RDONLY);
    IOBuf *from = zero->iob;
    Connection *null = fake_conn("/dev/null", O_WRONLY);
    IOBuf *to = null->iob;

    int rc = IOBuf_stream(from, to, 500);
    mu_assert(rc == 500, "Didn't stream the right amount on small test.");

    rc = IOBuf_stream(from, to, 10 * 1024);
    mu_assert(rc == 10 * 1024, "Didn't stream oversized amount.");

    fdclose(from->fd);
    rc = IOBuf_stream(from, to, 10 * 1024);
    mu_assert(rc == -1, "Should fail if send side is closed.");
    mu_assert(from->avail >= 0, "Avail should never go below 0.");

    fdclose(to->fd);
    rc = IOBuf_stream(from, to, 10 * 1024);
    mu_assert(rc == -1, "Should fail if recv side is closed.");

    fake_conn_close(zero);
    fake_conn_close(null);
    return NULL;
}

char * all_tests() {
    Register_init();
    mu_suite_start();

    mu_run_test(test_IOBuf_read_operations);
    mu_run_test(test_IOBuf_send_operations);
    mu_run_test(test_IOBuf_streaming);

    return NULL;
}

RUN_TESTS(all_tests);

