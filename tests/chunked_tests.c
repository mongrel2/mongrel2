#include "minunit.h"
#include <request.h>
#include <chunked.h>


char *test_chunked_can_read()
{
    int start;
    int len;
    int end;
    int rc;
    char *s1 = "c\r\nhello world\n\r\n";
    char *s2 = "00000000\r"; // 8 char size (need read)
    char *s3 = "00000000\r\n"; // 8 char size (need read)
    char *s4 = "000000000"; // 9 char size (need read, but soon to be error)
    char *s5 = "000000000\r"; // 9 char size (error)
    char *s6 = "000000000\r\n"; // 9 char size (error)
    char *s7 = "00000000\r\n\r\n"; // 8 char size
    char *s8 = "       0\r\n\r\n"; // 8 char size
    char *s9 = "c\r\nhello world\n\r";
    char *s10 = "c\r\nhello world\n\ra";
    char *s11 = "0\r\nFoo: Bar\r\nBar: Baz\r\n\r\n";

    rc = chunked_can_read(s1, strlen(s1), &start, &len, &end);
    mu_assert(rc == 1, "Wrong return value s1.");
    mu_assert(start == 3, "Wrong data_pos s1.");
    mu_assert(len == 12, "Wrong data_len s1.");
    mu_assert(end == 17, "Wrong end_pos s1.");

    rc = chunked_can_read(s2, strlen(s2), &start, &len, &end);
    mu_assert(rc == 0, "Wrong return value s2.");

    rc = chunked_can_read(s3, strlen(s3), &start, &len, &end);
    mu_assert(rc == 0, "Wrong return value s3.");

    rc = chunked_can_read(s4, strlen(s4), &start, &len, &end);
    mu_assert(rc == 0, "Wrong return value s4.");

    rc = chunked_can_read(s5, strlen(s5), &start, &len, &end);
    mu_assert(rc == -1, "Wrong return value s5.");

    rc = chunked_can_read(s6, strlen(s6), &start, &len, &end);
    mu_assert(rc == -1, "Wrong return value s6.");

    rc = chunked_can_read(s7, strlen(s7), &start, &len, &end);
    mu_assert(rc == 1, "Wrong return value s7.");
    mu_assert(start == 10, "Wrong data_pos s7.");
    mu_assert(len == 0, "Wrong data_len s7.");
    mu_assert(end == 12, "Wrong end_pos s7.");

    rc = chunked_can_read(s8, strlen(s8), &start, &len, &end);
    mu_assert(rc == 1, "Wrong return value s8.");
    mu_assert(start == 10, "Wrong data_pos s8.");
    mu_assert(len == 0, "Wrong data_len s8.");
    mu_assert(end == 12, "Wrong end_pos s8.");

    rc = chunked_can_read(s9, strlen(s9), &start, &len, &end);
    mu_assert(rc == 0, "Wrong return value s9.");

    rc = chunked_can_read(s10, strlen(s10), &start, &len, &end);
    mu_assert(rc == -1, "Wrong return value s10.");

    rc = chunked_can_read(s11, strlen(s11), &start, &len, &end);
    mu_assert(rc == 1, "Wrong return value s11.");
    mu_assert(start == 3, "Wrong data_pos s11.");
    mu_assert(len == 0, "Wrong data_len s11.");
    mu_assert(end == 25, "Wrong end_pos s11.");

    return NULL;
}

char *all_tests() {
    Request_init();
    mu_suite_start();

    mu_run_test(test_chunked_can_read);

    return NULL;
}

RUN_TESTS(all_tests);
