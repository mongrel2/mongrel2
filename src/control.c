
#line 1 "src/control.rl"
#include "control.h"
#include "bstring.h"
#include "task/task.h"
#include "register.h"
#include "dbg.h"
#include <stdlib.h>
#include <time.h>
#include "setting.h"


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

static inline void send_reply(void *sock, bstring rep)
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

static int CONTROL_RUNNING = 1;


#line 108 "src/control.rl"



#line 74 "src/control.c"
static const int ControlParser_start = 1;
static const int ControlParser_first_final = 34;
static const int ControlParser_error = 0;

static const int ControlParser_en_main = 1;


#line 111 "src/control.rl"

bstring Control_execute(bstring req)
{
    const char *p = bdata(req);
    const char *pe = p+blength(req);
    const char *mark = NULL;
    int cs = 0;
    bstring reply = NULL;

    debug("RECEIVED CONTROL COMMAND: %s", bdata(req));

    
#line 95 "src/control.c"
	{
	cs = ControlParser_start;
	}

#line 123 "src/control.rl"
    
#line 102 "src/control.c"
	{
	switch ( cs )
	{
case 1:
	switch( (*p) ) {
		case 99: goto st2;
		case 107: goto st13;
		case 115: goto st18;
		case 116: goto st31;
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
		goto tr15;
	goto st0;
tr15:
#line 74 "src/control.rl"
	{
        reply = bfromcstr("{\"msg\": \"stopping control port\"}");
        CONTROL_RUNNING = 0; {p++; cs = 34; goto _out;}
    }
	goto st34;
tr30:
#line 72 "src/control.rl"
	{ reply = Register_info(); {p++; cs = 34; goto _out;} }
	goto st34;
tr34:
#line 71 "src/control.rl"
	{ reply = taskgetinfo(); {p++; cs = 34; goto _out;} }
	goto st34;
tr37:
#line 79 "src/control.rl"
	{
        reply = bformat("{\"time\": %d}", (int)time(NULL)); {p++; cs = 34; goto _out;}
    }
	goto st34;
st34:
	p += 1;
case 34:
#line 207 "src/control.c"
	goto st0;
st13:
	p += 1;
case 13:
	if ( (*p) == 105 )
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
	if ( (*p) == 108 )
		goto st16;
	goto st0;
st16:
	p += 1;
case 16:
	if ( (*p) == 32 )
		goto st17;
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st17;
	goto st0;
st17:
	p += 1;
case 17:
	if ( (*p) == 32 )
		goto st17;
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr20;
	} else if ( (*p) >= 9 )
		goto st17;
	goto st0;
tr20:
#line 69 "src/control.rl"
	{ mark = p; }
#line 83 "src/control.rl"
	{
        int id = atoi(p);

        if(id < 0 || id > MAX_REGISTERED_FDS) {
            reply = bformat("{\"error\": \"invalid id: %d\"}", id);
        } else {
            int fd = Register_fd_for_id(id);

            if(fd > 0) {
                fdclose(fd);
                reply = bformat("{\"result\": \"killed %d\"}", id);
            } else {
                reply = bformat("{\"error\": \"does not exist: %d\"}", id);
            }
        }

        {p++; cs = 35; goto _out;}
    }
	goto st35;
tr38:
#line 83 "src/control.rl"
	{
        int id = atoi(p);

        if(id < 0 || id > MAX_REGISTERED_FDS) {
            reply = bformat("{\"error\": \"invalid id: %d\"}", id);
        } else {
            int fd = Register_fd_for_id(id);

            if(fd > 0) {
                fdclose(fd);
                reply = bformat("{\"result\": \"killed %d\"}", id);
            } else {
                reply = bformat("{\"error\": \"does not exist: %d\"}", id);
            }
        }

        {p++; cs = 35; goto _out;}
    }
	goto st35;
st35:
	p += 1;
case 35:
#line 293 "src/control.c"
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr38;
	goto st0;
st18:
	p += 1;
case 18:
	if ( (*p) == 116 )
		goto st19;
	goto st0;
st19:
	p += 1;
case 19:
	if ( (*p) == 97 )
		goto st20;
	goto st0;
st20:
	p += 1;
case 20:
	if ( (*p) == 116 )
		goto st21;
	goto st0;
st21:
	p += 1;
case 21:
	if ( (*p) == 117 )
		goto st22;
	goto st0;
st22:
	p += 1;
case 22:
	if ( (*p) == 115 )
		goto st23;
	goto st0;
st23:
	p += 1;
case 23:
	if ( (*p) == 32 )
		goto st24;
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st24;
	goto st0;
st24:
	p += 1;
case 24:
	switch( (*p) ) {
		case 32: goto st24;
		case 110: goto st25;
		case 116: goto st27;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st24;
	goto st0;
st25:
	p += 1;
case 25:
	if ( (*p) == 101 )
		goto st26;
	goto st0;
st26:
	p += 1;
case 26:
	if ( (*p) == 116 )
		goto tr30;
	goto st0;
st27:
	p += 1;
case 27:
	if ( (*p) == 97 )
		goto st28;
	goto st0;
st28:
	p += 1;
case 28:
	if ( (*p) == 115 )
		goto st29;
	goto st0;
st29:
	p += 1;
case 29:
	if ( (*p) == 107 )
		goto st30;
	goto st0;
st30:
	p += 1;
case 30:
	if ( (*p) == 115 )
		goto tr34;
	goto st0;
st31:
	p += 1;
case 31:
	if ( (*p) == 105 )
		goto st32;
	goto st0;
st32:
	p += 1;
case 32:
	if ( (*p) == 109 )
		goto st33;
	goto st0;
st33:
	p += 1;
case 33:
	if ( (*p) == 101 )
		goto tr37;
	goto st0;
	}

	_out: {}
	}

#line 124 "src/control.rl"

    check(p <= pe, "Buffer overflow after parsing.  Tell Zed that you sent something from a handler that went %ld past the end in the parser.", 
        (long int)(pe - p));

    if ( cs == 
#line 411 "src/control.c"
0
#line 128 "src/control.rl"
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

    void *sock = mqsocket(ZMQ_REP);
    check(sock != NULL, "Can't create contol socket.");

    rc = zmq_bind(sock, bdata(spec));
    check(rc == 0, "Failed to bind control port to run/control.");

    while(CONTROL_RUNNING) {
        taskstate("waiting");
        
        req = read_message(sock);
        check(req, "Failed to read message.");

        rep = Control_execute(req);

        taskstate("replying");
        send_reply(sock, rep);
        check(rep, "Faild to create a reply.");
    }

    log_info("Control port exiting.");
    taskexit(0);

error:
    taskexit(1);
}


void Control_port_start()
{
    taskcreate(Control_task, NULL, 32 * 1024);
}
