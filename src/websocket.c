#include <websocket.h>

struct tagbstring WS_REQ_METHOD = bsStatic("HYBI");
struct tagbstring WS_CONNECTION = bsStatic("connection");
struct tagbstring WS_UPGRADE = bsStatic("upgrade");
struct tagbstring WS_WEBSOCKET = bsStatic("websocket");
struct tagbstring WS_HOST = bsStatic("host");
struct tagbstring WS_SEC_WS_KEY = bsStatic("sec-websocket-key");
struct tagbstring WS_SEC_WS_VER = bsStatic("sec-websocket-version");
struct tagbstring WS_FLAGS = bsStatic("FLAGS");

bstring websocket_make_header(unsigned char flags,uint64_t length, int masked,unsigned char key[])
{
    static unsigned char buf[14];
    int where=0;
    if(masked) masked=0x80;
    buf[where++]=flags;
    if(length < 126) {
        buf[where++]=masked|length;
    }
    else if(length < 65536) {
        buf[where++]=masked|126;
        buf[where++]=length>>8;
        buf[where++]=length&0xff;
    }
    else {
        buf[where++]=masked|127;
        buf[where++]=(length>>56)&0xff;
        buf[where++]=(length>>48)&0xff;
        buf[where++]=(length>>40)&0xff;
        buf[where++]=(length>>32)&0xff;
        buf[where++]=(length>>24)&0xff;
        buf[where++]=(length>>16)&0xff;
        buf[where++]=(length>>8)&0xff;
        buf[where++]=(length)&0xff;
    }
    if(masked) {
        buf[where++]=key[0];
        buf[where++]=key[1];
        buf[where++]=key[2];
        buf[where++]=key[3];
    }
    return blk2bstr(buf,where);
}
