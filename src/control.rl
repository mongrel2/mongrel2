#include "control.h"
#include "bstring.h"
#include "task/task.h"
#include "register.h"
#include "server.h"
#include "dbg.h"
#include <stdlib.h>
#include <time.h>
#include "setting.h"
#include <signal.h>


extern Server *SERVER;

void bstring_free(void *data, void *hint)
{
    bdestroy((bstring)hint);
}

struct tagbstring REQ_STATUS_TASKS = bsStatic("status tasks");
struct tagbstring REQ_STATUS_NET = bsStatic("status net");


static inline bstring read_message(void *sock)
{
    bstring req = NULL;
    int rc = 0;

    zmq_msg_t *inmsg = calloc(sizeof(zmq_msg_t), 1);
    rc = zmq_msg_init(inmsg);
    check(rc == 0, "init failed.");

    rc = mqrecv(sock, inmsg, ZMQ_NOBLOCK);
    check(rc == 0, "Receive on control failed.");

    req = blk2bstr(zmq_msg_data(inmsg), zmq_msg_size(inmsg));
    check(req, "Failed to create the request string.");

    zmq_msg_close(inmsg);
    free(inmsg);

    return req;

error:
    if(inmsg) {
        zmq_msg_close(inmsg);
        free(inmsg);
    }

    return NULL;
}

static inline void send_reply(void *sock, bstring rep)
{
    int rc = 0;
    zmq_msg_t *outmsg = calloc(sizeof(zmq_msg_t), 1);
    rc = zmq_msg_init(outmsg);
    check(rc == 0, "init failed.");

    rc = zmq_msg_init_data(outmsg, bdata(rep), blength(rep), bstring_free, rep);
    check(rc == 0, "Failed to init reply data.");
    
    rc = mqsend(sock, outmsg, ZMQ_NOBLOCK);
    check(rc == 0, "Failed to deliver 0mq message to requestor.");

error:
    free(outmsg);
    return;
}

static int CONTROL_RUNNING = 1;
static void *CONTROL_SOCKET = NULL;


%%{
    machine ControlParser;

    action mark { mark = fpc; }

    action status_tasks { reply = taskgetinfo(); fbreak; }
    action status_net { reply = Register_info(); fbreak; }

    action control_stop {
        reply = bfromcstr("{\"msg\": \"stopping control port\"}");
        CONTROL_RUNNING = 0; fbreak;
    }

    action uuid {
        reply = bformat("{\"uuid\": \"%s\"}", bdata(SERVER->uuid)); fbreak;
    }

    action time {
        reply = bformat("{\"time\": %d}", (int)time(NULL)); fbreak;
    }

    action kill {
        int id = atoi(p);

        if(id < 0 || id > MAX_REGISTERED_FDS) {
            reply = bformat("{\"error\": \"invalid id: %d\"}", id);
        } else {
            int fd = Register_fd_for_id(id);

            if(fd >= 0) {
                Register_disconnect(fd);
                reply = bformat("{\"result\": \"killed %d\"}", id);
            } else {
                reply = bformat("{\"error\": \"does not exist: %d\"}", id);
            }
        }

        fbreak;
    }

    action reload {
        int rc = raise(SIGHUP);
        if (0 == rc) {
            reply = bfromcstr("{\"msg\": \"the server will be reloaded\"}");
        } else {
            reply = bfromcstr("{\"error\": \"failed to reload the server\"}");
        }
        fbreak;
    }

    action stop {
        // TODO: probably report back the number of waiting tasks
        int rc = raise(SIGINT);
        if (0 == rc) {
            reply = bfromcstr("{\"msg\": \"the server will be stopped\"}");
        } else {
            reply = bfromcstr("{\"error\": \"failed to stop the server\"}");
        }
        fbreak;
    }

    action terminate {
        // TODO: the server might have been terminated before
        // the reply is sent back. If this scenario is crucial (if possible at all)
        // find a workaround, otherwise, forget about it and remove this comment.
        int rc = raise(SIGTERM);
        if (0 == rc) {
            reply = bfromcstr("{\"msg\": \"the server will be terminated\"}");
        } else {
            reply = bfromcstr("{\"error\": \"failed to terminate the server\"}");
        }
        fbreak;
    }

    action help {
        reply = bfromcstr("{\"command_list\":[");
        bcatcstr(reply, "{\"name\": \"control stop\", \"description\": \"Close the control port.\"},\n");
        bcatcstr(reply, "{\"name\": \"kill <id>\", \"description\": \"Kill a connection in a violent way.\"},\n");
        bcatcstr(reply, "{\"name\": \"reload\", \"description\": \"Reload the server. Same as `kill -SIGHUP <server_pid>`.\"},\n");
        bcatcstr(reply, "{\"name\": \"status net\", \"description\": \"Return a list of active connections.\"},\n");
        bcatcstr(reply, "{\"name\": \"status tasks\", \"description\": \"Return a list of active tasks.\"},\n");
        bcatcstr(reply, "{\"name\": \"stop\", \"description\": \"Gracefully shut down the server. Same as `kill -SIGINT <server_pid>`.\"},\n");
        bcatcstr(reply, "{\"name\": \"terminate\", \"description\": \"Unconditionally terminate the server. Same as `kill -SIGTERM <server_pid>.\"},\n");
        bcatcstr(reply, "{\"name\": \"time\", \"description\": \"Return server timestamp.\"}\n");
        bcatcstr(reply, "{\"name\": \"uuid\", \"description\": \"Return server uuid.\"}\n");
        bcatcstr(reply, "]}\n");
        fbreak;
    }

    Status = "status" space+ ("tasks" @status_tasks | "net" @status_net);
    Control = "control stop" @control_stop;
    Time = "time" @time;
    UUID = "uuid" @uuid;
    Kill = "kill" space+ digit+ >mark @kill;
    Reload = "reload" @reload;
    Stop = "stop" @stop;
    Terminate = "terminate" @terminate;
    Help = "help" @help;

    main := Status | Control | Time | UUID | Kill | Reload | Stop | Terminate | Help;
}%%

%% write data;

bstring Control_execute(bstring req)
{
    const char *p = bdata(req);
    const char *pe = p+blength(req);
    const char *mark = NULL;
    int cs = 0;
    bstring reply = NULL;

    debug("RECEIVED CONTROL COMMAND: %s", bdata(req));

    %% write init;
    %% write exec noend;

    check(p <= pe, "Buffer overflow after parsing.  Tell Zed that you sent something from a handler that went %ld past the end in the parser.", 
        (long int)(pe - p));

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

struct tagbstring DEFAULT_CONTROL_SPEC = bsStatic("ipc://run/control");

void Control_task(void *v)
{
    int rc = 0;
    bstring req = NULL;
    bstring rep = NULL;
    bstring spec = Setting_get_str("control_port", &DEFAULT_CONTROL_SPEC);
    taskname("control");

    log_info("Setting up control socket in at %s", bdata(spec));

    CONTROL_SOCKET = mqsocket(ZMQ_REP);
    check(CONTROL_SOCKET != NULL, "Can't create contol socket.");

    rc = zmq_bind(CONTROL_SOCKET, bdata(spec));
    check(rc == 0, "Failed to bind control port to run/control.");

    while(CONTROL_RUNNING) {
        taskstate("waiting");
        
        req = read_message(CONTROL_SOCKET);
        check(req, "Failed to read message: %s.", strerror(errno));

        rep = Control_execute(req);
        bdestroy(req);

        taskstate("replying");
        send_reply(CONTROL_SOCKET, rep);
        check(rep, "Faild to create a reply.");
    }

    rc = zmq_close(CONTROL_SOCKET);
    check(rc == 0, "Failed to close control port socket.");
    CONTROL_SOCKET = NULL;
    log_info("Control port exiting.");
    taskexit(0);

error:
    log_info("Control port exiting with error.");
    zmq_close(CONTROL_SOCKET);
    CONTROL_SOCKET = NULL;
    taskexit(1);
}


void Control_port_start()
{
    taskcreate(Control_task, NULL, 32 * 1024);
}

int Control_port_stop()
{
    int rc = 0;

    if(CONTROL_SOCKET != NULL) {
        rc = zmq_close(CONTROL_SOCKET);
        check(rc == 0, "Failed to close control socket");

        // TODO: wake up the control task.
        // it doesn't seem to notice that the socket has been closed.
    }

error:
    return rc;
}
