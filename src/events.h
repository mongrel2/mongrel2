#ifndef _events_h
#define _events_h

typedef enum StateEvent {
    EVENT_START=100,
    ACCEPT=101,
    CLOSE=102,
    CONNECT=103,
    DIRECTORY=104,
    FAILED=105,
    HANDLER=106,
    HTTP_REQ=107,
    MSG_REQ=108,
    MSG_RESP=109,
    OPEN=110,
    PROXY=111,
    REMOTE_CLOSE=112,
    REQ_RECV=113,
    REQ_SENT=114,
    RESP_RECV=115,
    RESP_SENT=116,
    SOCKET_REQ=117,
    TIMEOUT=118,
    EVENT_END=119
} StateEvent ;




#endif
