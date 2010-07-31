#include "fuzzrnd.h"
#include "httpclient_parser.h"
#include <dbg.h>
#include <task/task.h>

FILE *LOG_FILE = NULL;


void element_debug(void *data, const char *at, size_t length)
{
    debug("ELEMENT: %.*s", length, at);
}


void field_debug(void *data, const char *field, size_t flen,
        const char *val, size_t vlen)
{
    debug("FIELD: %.*s = %.*s", flen, field, vlen, val);
}


void taskmain(int argc, char *argv[])
{
    int rc = 0;
    int fd = 0;
    int nread = 0;
    httpclient_parser parser;
    char buffer[10 * 1024];

    LOG_FILE = stderr;

    rc = httpclient_parser_init(&parser);
    parser.http_field = field_debug;
    parser.reason_phrase = element_debug;
    parser.status_code = element_debug;
    parser.chunk_size = element_debug;
    parser.http_version = element_debug;
    parser.header_done = element_debug;
    parser.last_chunk = element_debug;

    check(rc, "Failed to init parser.");

    fd = netdial(1, "mongrel2.org", 80);
    check(fd > 0, "Failed to connect to mongrel2.org.");

    char *request =  "GET / HTTP/1.1\r\nHost: mongrel2.org\r\nConnection: close\r\n\r\n";
    rc = fdsend(fd, request, strlen(request));
    check(rc == strlen(request), "Didn't send all of the request.");

    nread = fdrecv(fd, buffer, 10 * 1024 - 1);
    check(nread > 0, "Failed to read from server.");
    debug("Read %d from server.", nread);

    buffer[nread] = '\0';
    fdwrite(0, buffer, nread);

    size_t nparsed = httpclient_parser_execute(&parser, buffer, nread, 0);
    debug("Parsed %d", nparsed);
    check(httpclient_parser_finish(&parser) == 1, "Didn't parse.");

    taskexit(0);
error:

    taskexitall(1);
}
