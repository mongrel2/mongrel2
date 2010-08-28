
#line 1 "src/control.rl"
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



#line 71 "src/control.rl"



#line 70 "src/control.c"
static const int ControlParser_start = 1;
static const int ControlParser_first_final = 14;
static const int ControlParser_error = 0;

static const int ControlParser_en_main = 1;


#line 74 "src/control.rl"

bstring Control_execute(bstring req)
{
    const char *p = bdata(req);
    const char *pe = p+blength(req);
    int cs = 0;
    bstring reply = NULL;

    debug("RECEIVED CONTROL COMMAND: %s", bdata(req));

    
#line 90 "src/control.c"
	{
	cs = ControlParser_start;
	}

#line 85 "src/control.rl"
    
#line 97 "src/control.c"
	{
	switch ( cs )
	{
case 1:
	if ( (*p) == 115 )
		goto st2;
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	p += 1;
case 2:
	if ( (*p) == 116 )
		goto st3;
	goto st0;
st3:
	p += 1;
case 3:
	if ( (*p) == 97 )
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
	if ( (*p) == 117 )
		goto st6;
	goto st0;
st6:
	p += 1;
case 6:
	if ( (*p) == 115 )
		goto st7;
	goto st0;
st7:
	p += 1;
case 7:
	switch( (*p) ) {
		case 32: goto st7;
		case 110: goto st8;
		case 116: goto st10;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st7;
	goto st0;
st8:
	p += 1;
case 8:
	if ( (*p) == 101 )
		goto st9;
	goto st0;
st9:
	p += 1;
case 9:
	if ( (*p) == 116 )
		goto tr10;
	goto st0;
tr10:
#line 66 "src/control.rl"
	{ reply = bfromcstr("not ready"); {p++; cs = 14; goto _out;} }
	goto st14;
tr14:
#line 65 "src/control.rl"
	{ reply = taskgetinfo(); {p++; cs = 14; goto _out;} }
	goto st14;
st14:
	p += 1;
case 14:
#line 172 "src/control.c"
	goto st0;
st10:
	p += 1;
case 10:
	if ( (*p) == 97 )
		goto st11;
	goto st0;
st11:
	p += 1;
case 11:
	if ( (*p) == 115 )
		goto st12;
	goto st0;
st12:
	p += 1;
case 12:
	if ( (*p) == 107 )
		goto st13;
	goto st0;
st13:
	p += 1;
case 13:
	if ( (*p) == 115 )
		goto tr14;
	goto st0;
	}

	_out: {}
	}

#line 86 "src/control.rl"

    check(p <= pe, "Buffer overflow after parsing.  Tell Zed that you sent something from a handler that went %ld past the end in the parser.", pe - p);

    if ( cs == 
#line 208 "src/control.c"
0
#line 89 "src/control.rl"
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
