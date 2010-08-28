#include "control.h"
#include "bstring.h"
#include "task/task.h"
#include "dbg.h"
#include <stdlib.h>


void bstring_free(void *data, void *hint)
{
    bdestroy((bstring)hint);
}

struct tagbstring REQ_STATUS_TASKS = bsStatic("status tasks");
struct tagbstring REQ_STATUS_NET = bsStatic("status net");


inline bstring read_message(void *sock)
{
    bstring req = NULL;
    int rc = 0;

    zmq_msg_t *inmsg = calloc(sizeof(zmq_msg_t), 1);
    check_mem(inmsg);
    rc = zmq_msg_init(inmsg);
    check(rc == 0, "init failed.");

    rc = mqrecv(sock, inmsg, ZMQ_NOBLOCK);
    check(rc == 0, "Receive on control failed.");

    req = blk2bstr(zmq_msg_data(inmsg), zmq_msg_size(inmsg));
    check(req, "Failed to create the request string.");

    zmq_msg_close(inmsg);

    return req;

error:
    return NULL;
}

inline void send_reply(void *sock, bstring rep)
{
    int rc = 0;
    zmq_msg_t *outmsg = NULL;

    outmsg = calloc(sizeof(zmq_msg_t), 1);
    check_mem(outmsg);
    rc = zmq_msg_init(outmsg);
    check(rc == 0, "init failed.");

    rc = zmq_msg_init_data(outmsg, bdata(rep), blength(rep), bstring_free, rep);
    check(rc == 0, "Failed to init reply data.");
    
    rc = mqsend(sock, outmsg, 0);
    check(rc == 0, "Failed to deliver 0mq message to requestor.");

error:
    return;
}


%%{
    machine ControlParser;

    action status_tasks { reply = taskgetinfo(); fbreak; }
    action status_net { reply = bfromcstr("not ready"); fbreak; }

    StatusCmd = "status" space* ("tasks" @status_tasks | "net" @status_net);

    main := StatusCmd;
}%%

%% write data;

bstring Control_execute(bstring req)
{
    const char *p = bdata(req);
    const char *pe = p+blength(req);
    int cs = 0;
    bstring reply = NULL;

    debug("RECEIVED CONTROL COMMAND: %s", bdata(req));

    %% write init;
    %% write exec noend;

    check(p <= pe, "Buffer overflow after parsing.  Tell Zed that you sent something from a handler that went %ld past the end in the parser.", pe - p);

    if ( cs == %%{ write error; }%% ) {
        check(pe - p > 0, "Major erorr in the parser, tell Zed.");
        return bformat("{\"error\": \"parsing error at: ...%s\"}", bdata(req) + (pe - p));
    } else if(!reply) {
        return bformat("{\"error\": \"invalid command\"}", pe - p);
    } else {
        return reply;
    }

error:
    return bfromcstr("{\"error\": \"fatal error\"}");
}


void Control_task(void *v)
{
    int rc = 0;
    bstring req = NULL;
    bstring rep = NULL;
    taskname("control");

    debug("Setting up control socket in run/control");
    void *sock = mqsocket(ZMQ_REP);
    check(sock != NULL, "Can't create contol socket.");

    rc = zmq_bind(sock, "ipc://run/control");
    check(rc == 0, "Failed to bind control port to run/control.");

    while(1) {
        taskstate("waiting");
        
        req = read_message(sock);
        check(req, "Failed to read message.");

        rep = Control_execute(req);

        taskstate("replying");
        send_reply(sock, rep);
        check(rep, "Faild to create a reply.");
    }

    taskexit(0);

error:
    taskexit(1);
}


void Control_port_start()
{
    taskcreate(Control_task, NULL, 32 * 1024);
}
