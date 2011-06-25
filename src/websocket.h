#ifndef _websocket_h
#define _websocket_h

#include <bstring.h>

extern struct tagbstring WS_CONNECTION;
extern struct tagbstring WS_UPGRADE;
extern struct tagbstring WS_WEBSOCKET;
extern struct tagbstring WS_HOST;
extern struct tagbstring WS_SEC_WS_KEY;
extern struct tagbstring WS_SEC_WS_VER;

enum Ws_Flags {
 WSFLAG_CONN=1,
 WSFLAG_UPGRADE=2,
 WSFLAG_HOST=4,
 WSFLAG_SEC_KEY=8,
 WSFLAG_SEC_VER=16
};
#endif

