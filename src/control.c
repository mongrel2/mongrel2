
#line 1 "control.rl"
#include "control.h"
#include "bstring.h"
#include "task/task.h"
#include "register.h"
#include "dbg.h"
#include <stdlib.h>
#include <time.h>
#include "setting.h"
#include <signal.h>


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
    
    rc = mqsend(sock, outmsg, 0);
    check(rc == 0, "Failed to deliver 0mq message to requestor.");

error:
    free(outmsg);
    return;
}

static int CONTROL_RUNNING = 1;


#line 164 "control.rl"



#line 78 "control.c"
static const char _ControlParser_actions[] = {
	0, 1, 1, 1, 2, 1, 3, 1, 
	4, 1, 5, 1, 6, 1, 7, 1, 
	8, 1, 9, 2, 0, 5
};

static const char _ControlParser_key_offsets[] = {
	0, 0, 6, 7, 8, 9, 10, 11, 
	12, 13, 14, 15, 16, 17, 18, 19, 
	20, 21, 22, 23, 26, 31, 32, 33, 
	34, 35, 36, 37, 39, 40, 41, 42, 
	45, 50, 51, 52, 53, 54, 55, 56, 
	57, 59, 60, 61, 62, 63, 64, 65, 
	66, 67, 68, 68
};

static const char _ControlParser_trans_keys[] = {
	99, 104, 107, 114, 115, 116, 111, 110, 
	116, 114, 111, 108, 32, 115, 116, 111, 
	112, 101, 108, 112, 105, 108, 108, 32, 
	9, 13, 32, 9, 13, 48, 57, 101, 
	108, 111, 97, 100, 116, 97, 111, 116, 
	117, 115, 32, 9, 13, 32, 110, 116, 
	9, 13, 101, 116, 97, 115, 107, 115, 
	112, 101, 105, 114, 109, 105, 110, 97, 
	116, 101, 109, 101, 48, 57, 0
};

static const char _ControlParser_single_lengths[] = {
	0, 6, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 1, 2, 1, 1, 1, 1, 
	3, 1, 1, 1, 1, 1, 1, 1, 
	2, 1, 1, 1, 1, 1, 1, 1, 
	1, 1, 0, 0
};

static const char _ControlParser_range_lengths[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1, 2, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	1, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 1
};

static const unsigned char _ControlParser_index_offsets[] = {
	0, 0, 7, 9, 11, 13, 15, 17, 
	19, 21, 23, 25, 27, 29, 31, 33, 
	35, 37, 39, 41, 44, 48, 50, 52, 
	54, 56, 58, 60, 63, 65, 67, 69, 
	72, 77, 79, 81, 83, 85, 87, 89, 
	91, 94, 96, 98, 100, 102, 104, 106, 
	108, 110, 112, 113
};

static const char _ControlParser_trans_targs[] = {
	2, 13, 16, 21, 26, 40, 0, 3, 
	0, 4, 0, 5, 0, 6, 0, 7, 
	0, 8, 0, 9, 0, 10, 0, 11, 
	0, 12, 0, 50, 0, 14, 0, 15, 
	0, 50, 0, 17, 0, 18, 0, 19, 
	0, 20, 20, 0, 20, 20, 51, 0, 
	22, 0, 23, 0, 24, 0, 25, 0, 
	50, 0, 27, 0, 28, 39, 0, 29, 
	0, 30, 0, 31, 0, 32, 32, 0, 
	32, 33, 35, 32, 0, 34, 0, 50, 
	0, 36, 0, 37, 0, 38, 0, 50, 
	0, 50, 0, 41, 48, 0, 42, 0, 
	43, 0, 44, 0, 45, 0, 46, 0, 
	47, 0, 50, 0, 49, 0, 50, 0, 
	0, 51, 0, 0
};

static const char _ControlParser_trans_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 5, 0, 0, 0, 0, 
	0, 17, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 19, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	11, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 3, 
	0, 0, 0, 0, 0, 0, 0, 1, 
	0, 13, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 15, 0, 0, 0, 7, 0, 
	0, 9, 0, 0
};

static const int ControlParser_start = 1;
static const int ControlParser_first_final = 50;
static const int ControlParser_error = 0;

static const int ControlParser_en_main = 1;


#line 167 "control.rl"

bstring Control_execute(bstring req)
{
    const char *p = bdata(req);
    const char *pe = p+blength(req);
    const char *mark = NULL;
    int cs = 0;
    bstring reply = NULL;

    debug("RECEIVED CONTROL COMMAND: %s", bdata(req));

    
#line 193 "control.c"
	{
	cs = ControlParser_start;
	}

#line 179 "control.rl"
    
#line 200 "control.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( cs == 0 )
		goto _out;
_resume:
	_keys = _ControlParser_trans_keys + _ControlParser_key_offsets[cs];
	_trans = _ControlParser_index_offsets[cs];

	_klen = _ControlParser_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _ControlParser_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += ((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	cs = _ControlParser_trans_targs[_trans];

	if ( _ControlParser_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _ControlParser_actions + _ControlParser_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 73 "control.rl"
	{ mark = p; }
	break;
	case 1:
#line 75 "control.rl"
	{ reply = taskgetinfo(); {p++; goto _out; } }
	break;
	case 2:
#line 76 "control.rl"
	{ reply = Register_info(); {p++; goto _out; } }
	break;
	case 3:
#line 78 "control.rl"
	{
        reply = bfromcstr("{\"msg\": \"stopping control port\"}");
        CONTROL_RUNNING = 0; {p++; goto _out; }
    }
	break;
	case 4:
#line 83 "control.rl"
	{
        reply = bformat("{\"time\": %d}", (int)time(NULL)); {p++; goto _out; }
    }
	break;
	case 5:
#line 87 "control.rl"
	{
        int id = atoi(p);

        if(id < 0 || id > MAX_REGISTERED_FDS) {
            reply = bformat("{\"error\": \"invalid id: %d\"}", id);
        } else {
            int fd = Register_fd_for_id(id);

            if(fd >= 0) {
                fdclose(fd);
                reply = bformat("{\"result\": \"killed %d\"}", id);
            } else {
                reply = bformat("{\"error\": \"does not exist: %d\"}", id);
            }
        }

        {p++; goto _out; }
    }
	break;
	case 6:
#line 106 "control.rl"
	{
        int rc = raise(SIGHUP);
        if (0 == rc) {
            reply = bfromcstr("{\"msg\": \"the server will be reloaded\"}");
        } else {
            reply = bfromcstr("{\"error\": \"failed to reload the server\"}");
        }
        {p++; goto _out; }
    }
	break;
	case 7:
#line 116 "control.rl"
	{
        // TODO: probably report back the number of waiting tasks
        int rc = raise(SIGINT);
        if (0 == rc) {
            reply = bfromcstr("{\"msg\": \"the server will be stopped\"}");
        } else {
            reply = bfromcstr("{\"error\": \"failed to stop the server\"}");
        }
        {p++; goto _out; }
    }
	break;
	case 8:
#line 127 "control.rl"
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
        {p++; goto _out; }
    }
	break;
	case 9:
#line 140 "control.rl"
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
        bcatcstr(reply, "]}\n");
        {p++; goto _out; }
    }
	break;
#line 373 "control.c"
		}
	}

_again:
	if ( cs == 0 )
		goto _out;
	p += 1;
	goto _resume;
	_out: {}
	}

#line 180 "control.rl"

    check(p <= pe, "Buffer overflow after parsing.  Tell Zed that you sent something from a handler that went %ld past the end in the parser.", 
        (long int)(pe - p));

    if ( cs == 
#line 391 "control.c"
0
#line 184 "control.rl"
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
