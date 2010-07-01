#ifndef _events_h
#define _events_h

enum {
    OPEN=1,
    CLOSE=2,
    REQ_RECV=3,
    RESP_RECV=4,
    REQ_SENT=5,
    RESP_SENT=6,
    ACCEPT=7,
    JSON_REQ=8,
    SOCKET_REQ=9,
    HTTP_REQ=10,
    FAILED=11,
    CONNECT=12,
    PARSED=13,
    HANDLER=14,
    DIRECTORY=15,
    PROXY=16,
    REMOTE_CLOSE=17
};

#endif
