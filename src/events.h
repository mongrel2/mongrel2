#ifndef _events_h
#define _events_h

typedef enum StateEvent {
    FINISHED=100,
    ACCEPT=101,
    CLOSE=102,
    CONNECT=103,
    DIRECTORY=104,
    FAILED=105,
    HANDLER=106,
    HTTP_REQ=107,
    MSG_REQ=108,
    OPEN=109,
    PROXY=110,
    REMOTE_CLOSE=111,
    REQ_RECV=112,
    REQ_SENT=113,
    RESP_SENT=114,
    SOCKET_REQ=115,
    TIMEOUT=116,
    EVENT_END=117
} StateEvent ;


#endif
