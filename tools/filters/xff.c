#include <filter.h>
#include <dbg.h>
#include <tnetstrings.h>

extern int CLIENT_READ_RETRIES;

StateEvent filter_transition(StateEvent state, Connection *conn, tns_value_t *config)
{
    size_t len = 0;
    char *data = tns_render(config, &len);

    int next;
    int rc = 0;
    int req_len = Request_header_length(conn->req);
    int cont_len = Request_content_length(conn->req);
    int total_len = req_len+cont_len;
    bstring xff;
    char *xffBuf;
    unsigned sofar=0;

    /* This is a Hack, it appears valid though,
     * as this buffer is allocated when entering proxy
     * and freed when leaving */
    if(conn->proxy_iob == NULL ) {
        return state;
    }

    char *buf = IOBuf_read_all(conn->iob, total_len, CLIENT_READ_RETRIES);
    check(buf != NULL, "Failed to read from the client socket to proxy.");

    rc = IOBuf_send(conn->proxy_iob, IOBuf_start(conn->iob), req_len-2);
    check(rc > 0, "Failed to send to proxy.");

    xff = Request_get(conn->req, &HTTP_X_FORWARDED_FOR);

    xffBuf = malloc(blength(&HTTP_X_FORWARDED_FOR)+2+blength(xff)+2);
    memcpy(xffBuf,bdata(&HTTP_X_FORWARDED_FOR),blength(&HTTP_X_FORWARDED_FOR));
    sofar+=blength(&HTTP_X_FORWARDED_FOR);

    memcpy(xffBuf+sofar,": ",2);
    sofar+=2;

    memcpy(xffBuf+sofar,bdata(xff),blength(xff));
    sofar+=blength(xff);

    memcpy(xffBuf+sofar,"\r\n",2);
    sofar+=2;

    rc = IOBuf_send(conn->proxy_iob, xffBuf, sofar);
    check(rc > 0, "Failed to send to proxy.");

    rc = IOBuf_send(conn->proxy_iob, IOBuf_start(conn->iob)+req_len-2, cont_len+2);
    check(rc > 0, "Failed to send to proxy.");

    next = State_exec(&conn->state, REQ_SENT, (void *)conn);

error:
    next = State_exec(&conn->state, REMOTE_CLOSE, (void *)conn);
}   
       



StateEvent *filter_init(Server *srv, bstring load_path, int *out_nstates)
{
    StateEvent states[] = {HTTP_REQ,CONNECT};
    *out_nstates = Filter_states_length(states);
    check(*out_nstates == 2, "Wrong state array length.");

    return Filter_state_list(states, *out_nstates);

error:
    return NULL;
}

