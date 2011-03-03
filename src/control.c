
#line 1 "src/control.rl"
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



#line 175 "src/control.rl"



#line 83 "src/control.c"
static const int ControlParser_start = 1;
static const int ControlParser_first_final = 53;
static const int ControlParser_error = 0;

static const int ControlParser_en_main = 1;


#line 178 "src/control.rl"

bstring Control_execute(bstring req)
{
    const char *p = bdata(req);
    const char *pe = p+blength(req);
    const char *mark = NULL;
    int cs = 0;
    bstring reply = NULL;

    debug("RECEIVED CONTROL COMMAND: %s", bdata(req));

    
#line 104 "src/control.c"
	{
	cs = ControlParser_start;
	}

#line 190 "src/control.rl"
    
#line 111 "src/control.c"
	{
	switch ( cs )
	{
case 1:
	switch( (*p) ) {
		case 99: goto st2;
		case 104: goto st13;
		case 107: goto st16;
		case 114: goto st21;
		case 115: goto st26;
		case 116: goto st40;
		case 117: goto st50;
	}
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	p += 1;
case 2:
	if ( (*p) == 111 )
		goto st3;
	goto st0;
st3:
	p += 1;
case 3:
	if ( (*p) == 110 )
		goto st4;
	goto st0;
st4:
	p += 1;
case 4:
	if ( (*p) == 116 )
		goto st5;
	goto st0;
st5:
	p += 1;
case 5:
	if ( (*p) == 114 )
		goto st6;
	goto st0;
st6:
	p += 1;
case 6:
	if ( (*p) == 111 )
		goto st7;
	goto st0;
st7:
	p += 1;
case 7:
	if ( (*p) == 108 )
		goto st8;
	goto st0;
st8:
	p += 1;
case 8:
	if ( (*p) == 32 )
		goto st9;
	goto st0;
st9:
	p += 1;
case 9:
	if ( (*p) == 115 )
		goto st10;
	goto st0;
st10:
	p += 1;
case 10:
	if ( (*p) == 116 )
		goto st11;
	goto st0;
st11:
	p += 1;
case 11:
	if ( (*p) == 111 )
		goto st12;
	goto st0;
st12:
	p += 1;
case 12:
	if ( (*p) == 112 )
		goto tr18;
	goto st0;
tr18:
#line 83 "src/control.rl"
	{
        reply = bfromcstr("{\"msg\": \"stopping control port\"}");
        CONTROL_RUNNING = 0; {p++; cs = 53; goto _out;}
    }
	goto st53;
tr21:
#line 149 "src/control.rl"
	{
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
        {p++; cs = 53; goto _out;}
    }
	goto st53;
tr31:
#line 115 "src/control.rl"
	{
        int rc = raise(SIGHUP);
        if (0 == rc) {
            reply = bfromcstr("{\"msg\": \"the server will be reloaded\"}");
        } else {
            reply = bfromcstr("{\"error\": \"failed to reload the server\"}");
        }
        {p++; cs = 53; goto _out;}
    }
	goto st53;
tr42:
#line 81 "src/control.rl"
	{ reply = Register_info(); {p++; cs = 53; goto _out;} }
	goto st53;
tr46:
#line 80 "src/control.rl"
	{ reply = taskgetinfo(); {p++; cs = 53; goto _out;} }
	goto st53;
tr47:
#line 125 "src/control.rl"
	{
        // TODO: probably report back the number of waiting tasks
        int rc = raise(SIGINT);
        if (0 == rc) {
            reply = bfromcstr("{\"msg\": \"the server will be stopped\"}");
        } else {
            reply = bfromcstr("{\"error\": \"failed to stop the server\"}");
        }
        {p++; cs = 53; goto _out;}
    }
	goto st53;
tr56:
#line 136 "src/control.rl"
	{
        // TODO: the server might have been terminated before
        // the reply is sent back. If this scenario is crucial (if possible at all)
        // find a workaround, otherwise, forget about it and remove this comment.
        int rc = raise(SIGTERM);
        if (0 == rc) {
            reply = bfromcstr("{\"msg\": \"the server will be terminated\"}");
        } else {
            reply = bfromcstr("{\"error\": \"failed to terminate the server\"}");
        }
        {p++; cs = 53; goto _out;}
    }
	goto st53;
tr58:
#line 92 "src/control.rl"
	{
        reply = bformat("{\"time\": %d}", (int)time(NULL)); {p++; cs = 53; goto _out;}
    }
	goto st53;
tr61:
#line 88 "src/control.rl"
	{
        reply = bformat("{\"uuid\": \"%s\"}", bdata(SERVER->uuid)); {p++; cs = 53; goto _out;}
    }
	goto st53;
st53:
	p += 1;
case 53:
#line 282 "src/control.c"
	goto st0;
st13:
	p += 1;
case 13:
	if ( (*p) == 101 )
		goto st14;
	goto st0;
st14:
	p += 1;
case 14:
	if ( (*p) == 108 )
		goto st15;
	goto st0;
st15:
	p += 1;
case 15:
	if ( (*p) == 112 )
		goto tr21;
	goto st0;
st16:
	p += 1;
case 16:
	if ( (*p) == 105 )
		goto st17;
	goto st0;
st17:
	p += 1;
case 17:
	if ( (*p) == 108 )
		goto st18;
	goto st0;
st18:
	p += 1;
case 18:
	if ( (*p) == 108 )
		goto st19;
	goto st0;
st19:
	p += 1;
case 19:
	if ( (*p) == 32 )
		goto st20;
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st20;
	goto st0;
st20:
	p += 1;
case 20:
	if ( (*p) == 32 )
		goto st20;
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr26;
	} else if ( (*p) >= 9 )
		goto st20;
	goto st0;
tr26:
#line 78 "src/control.rl"
	{ mark = p; }
#line 96 "src/control.rl"
	{
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

        {p++; cs = 54; goto _out;}
    }
	goto st54;
tr62:
#line 96 "src/control.rl"
	{
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

        {p++; cs = 54; goto _out;}
    }
	goto st54;
st54:
	p += 1;
case 54:
#line 386 "src/control.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr62;
	goto st0;
st21:
	p += 1;
case 21:
	if ( (*p) == 101 )
		goto st22;
	goto st0;
st22:
	p += 1;
case 22:
	if ( (*p) == 108 )
		goto st23;
	goto st0;
st23:
	p += 1;
case 23:
	if ( (*p) == 111 )
		goto st24;
	goto st0;
st24:
	p += 1;
case 24:
	if ( (*p) == 97 )
		goto st25;
	goto st0;
st25:
	p += 1;
case 25:
	if ( (*p) == 100 )
		goto tr31;
	goto st0;
st26:
	p += 1;
case 26:
	if ( (*p) == 116 )
		goto st27;
	goto st0;
st27:
	p += 1;
case 27:
	switch( (*p) ) {
		case 97: goto st28;
		case 111: goto st39;
	}
	goto st0;
st28:
	p += 1;
case 28:
	if ( (*p) == 116 )
		goto st29;
	goto st0;
st29:
	p += 1;
case 29:
	if ( (*p) == 117 )
		goto st30;
	goto st0;
st30:
	p += 1;
case 30:
	if ( (*p) == 115 )
		goto st31;
	goto st0;
st31:
	p += 1;
case 31:
	if ( (*p) == 32 )
		goto st32;
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st32;
	goto st0;
st32:
	p += 1;
case 32:
	switch( (*p) ) {
		case 32: goto st32;
		case 110: goto st33;
		case 116: goto st35;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st32;
	goto st0;
st33:
	p += 1;
case 33:
	if ( (*p) == 101 )
		goto st34;
	goto st0;
st34:
	p += 1;
case 34:
	if ( (*p) == 116 )
		goto tr42;
	goto st0;
st35:
	p += 1;
case 35:
	if ( (*p) == 97 )
		goto st36;
	goto st0;
st36:
	p += 1;
case 36:
	if ( (*p) == 115 )
		goto st37;
	goto st0;
st37:
	p += 1;
case 37:
	if ( (*p) == 107 )
		goto st38;
	goto st0;
st38:
	p += 1;
case 38:
	if ( (*p) == 115 )
		goto tr46;
	goto st0;
st39:
	p += 1;
case 39:
	if ( (*p) == 112 )
		goto tr47;
	goto st0;
st40:
	p += 1;
case 40:
	switch( (*p) ) {
		case 101: goto st41;
		case 105: goto st48;
	}
	goto st0;
st41:
	p += 1;
case 41:
	if ( (*p) == 114 )
		goto st42;
	goto st0;
st42:
	p += 1;
case 42:
	if ( (*p) == 109 )
		goto st43;
	goto st0;
st43:
	p += 1;
case 43:
	if ( (*p) == 105 )
		goto st44;
	goto st0;
st44:
	p += 1;
case 44:
	if ( (*p) == 110 )
		goto st45;
	goto st0;
st45:
	p += 1;
case 45:
	if ( (*p) == 97 )
		goto st46;
	goto st0;
st46:
	p += 1;
case 46:
	if ( (*p) == 116 )
		goto st47;
	goto st0;
st47:
	p += 1;
case 47:
	if ( (*p) == 101 )
		goto tr56;
	goto st0;
st48:
	p += 1;
case 48:
	if ( (*p) == 109 )
		goto st49;
	goto st0;
st49:
	p += 1;
case 49:
	if ( (*p) == 101 )
		goto tr58;
	goto st0;
st50:
	p += 1;
case 50:
	if ( (*p) == 117 )
		goto st51;
	goto st0;
st51:
	p += 1;
case 51:
	if ( (*p) == 105 )
		goto st52;
	goto st0;
st52:
	p += 1;
case 52:
	if ( (*p) == 100 )
		goto tr61;
	goto st0;
	}

	_out: {}
	}

#line 191 "src/control.rl"

    check(p <= pe, "Buffer overflow after parsing.  Tell Zed that you sent something from a handler that went %ld past the end in the parser.", 
        (long int)(pe - p));

    if ( cs == 
#line 604 "src/control.c"
0
#line 195 "src/control.rl"
 ) {
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
