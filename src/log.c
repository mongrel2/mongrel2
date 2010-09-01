#include "log.h"

#include <stdio.h>
#include "dbg.h"
#include "request.h"
#include "headers.h"
#include "setting.h"

FILE *ACCESS_LOG = NULL;

int Log_init(Server *server)
{
    if(ACCESS_LOG == NULL) {
        if(Setting_get_int("disable.access_logging", 0)) {
            log_info("Access log is disabled according to disable.access_logging.");
        } else {
            ACCESS_LOG = fopen(bdata(server->access_log), "a+");
            check(ACCESS_LOG != NULL, "Failed to open the access log: %s", bdata(server->access_log));
            setbuf(ACCESS_LOG, NULL);
        }
    }

    return 0;
error:
    ACCESS_LOG = stderr;
    return -1;
}

int Log_request(Connection *conn, int status, int size)
{
    Request *req = conn->req;

    if(ACCESS_LOG) {
        // TODO: there's a security hole here in that people can inject xterm codes from this
        fprintf(ACCESS_LOG, "%s %.*s -- -- %d \"%s %s %s\" %d %d\n",
                bdata(req->target_host->name),
                IPADDR_SIZE, conn->remote,
                (int)time(NULL),
                Request_is_json(req) ? "JSON" : bdata(req->request_method),
                Request_is_json(req) ? bdata(Request_path(req)) : bdata(req->uri),
                Request_is_json(req) ? "" : bdata(req->version),
                status,
                size);
    }

    return 0;
}
