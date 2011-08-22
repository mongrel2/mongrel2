#include <filter.h>
#include <dbg.h>
#include <tnetstrings.h>

extern int CLIENT_READ_RETRIES;

StateEvent filter_transition(StateEvent state, Connection *conn, tns_value_t *config)
{
    size_t len = 0;

    int next;
    int rc = 0;
    int req_len = Request_header_length(conn->req);
    int cont_len = Request_content_length(conn->req);
    int total_len = req_len+cont_len;
    bstring xff;

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


    xff=bstrcpy(&HTTP_X_FORWARDED_FOR);
    check_mem(xff)
    rc = bcatcstr(xff,": ");
    check(rc == BSTR_OK, "Out of memory");
    rc = bconcat(xff,Request_get(conn->req, &HTTP_X_FORWARDED_FOR));
    check(rc == BSTR_OK, "Out of memory");
    rc = bcatcstr(xff,"\r\n");
    check(rc == BSTR_OK, "Out of memory");

    rc = IOBuf_send(conn->proxy_iob, bdata(xff), blength(xff));
    check(rc > 0, "Failed to send to proxy.");

    rc = IOBuf_send(conn->proxy_iob, IOBuf_start(conn->iob)+req_len-2, cont_len+2);
    check(rc > 0, "Failed to send to proxy.");

    next = State_exec(&conn->state, REQ_SENT, (void *)conn);

error:
    next = State_exec(&conn->state, REMOTE_CLOSE, (void *)conn);
    return next;
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

