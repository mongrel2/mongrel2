#include <filter.h>
#include <dbg.h>
#include <tnetstrings.h>

static struct tagbstring rewritePath=bsStatic("/proxy/");
static struct tagbstring newPath=bsStatic("/");

StateEvent filter_transition(StateEvent state, Connection *conn, tns_value_t *config)
{

    log_info("REWRITE: %s", bdata(conn->req->path));
    if(0==bstrncmp(conn->req->path,&rewritePath,blength(&rewritePath))) {
        bstring header = bfromcstralloc(1024,"");
        hscan_t scan;
        hnode_t *n = NULL;
        Request *req = conn->req;
        bstring newpath=bstrcpy(conn->req->path);

        bconcat(header,req->request_method);
        bconchar(header,' ');
        breplace(newpath,0,blength(&rewritePath),&newPath,0);
        //bconcat(header,req->path);
        bconcat(header,newpath);
        bdestroy(newpath);
        newpath=NULL;
        bconchar(header,' ');
        bconcat(header,req->version);
        bcatcstr(header,"\r\n");

        hash_scan_begin(&scan,req->headers);
        for(n = hash_scan_next(&scan); n != NULL; n = hash_scan_next(&scan)) {
            struct bstrList *val_list = hnode_get(n);
            bstring key;
            int i;
            if(val_list->qty == 0) continue;
            key = (bstring)hnode_getkey(n);
            bconcat(header,key);
            bconchar(header,':');
            bconcat(header,val_list->entry[0]);
            for(i=1;i<val_list->qty;++i) {
                bconchar(header,',');
                bconcat(header,val_list->entry[i]);
            }
            bcatcstr(header,"\r\n");
        }
        bcatcstr(header,"\r\n");

        req->new_header=header;
    }

    return state;
}


StateEvent *filter_init(Server *srv, bstring load_path, int *out_nstates)
{
    StateEvent states[] = {PROXY};
    *out_nstates = Filter_states_length(states);
    check(*out_nstates == 1, "Wrong state array length.");

    return Filter_state_list(states, *out_nstates);

error:
    return NULL;
}

