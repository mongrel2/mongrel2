#include <websocket.h>
#include <dbg.h>
#include <mbedtls/sha1.h>

struct tagbstring WS_REQ_METHOD = bsStatic("HYBI");
struct tagbstring WS_CONNECTION = bsStatic("connection");
struct tagbstring WS_UPGRADE = bsStatic("upgrade");
struct tagbstring WS_WEBSOCKET = bsStatic("websocket");
struct tagbstring WS_HOST = bsStatic("host");
struct tagbstring WS_SEC_WS_KEY = bsStatic("sec-websocket-key");
struct tagbstring WS_SEC_WS_VER = bsStatic("sec-websocket-version");
struct tagbstring WS_FLAGS = bsStatic("FLAGS");

#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

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

bstring websocket_challenge(bstring input)
{
    bstring buf=NULL;
    bstring tmpstring=NULL;
    bstring encodedSha1=NULL;

    buf = bfromcstralloc(20,"");
    check_mem(buf);

    tmpstring=bstrcpy(input);
    check_mem(tmpstring);

    check(BSTR_OK == bcatcstr(tmpstring, WS_GUID),"Failed to allocate memory");

    mbedtls_sha1((unsigned char *)bdata(tmpstring),blength(tmpstring),(unsigned char *)bdata(buf));
    buf->slen=20;
    encodedSha1=bBase64Encode(buf);

    bdestroy(tmpstring);
    bdestroy(buf);

/*
    tmpstring = bformat("HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n\r\n",bdata(encodedSha1));
    bdestroy(encodedSha1);
    */
    return encodedSha1;

error:
    bdestroy(buf);
    bdestroy(tmpstring);
    bdestroy(encodedSha1);
    return NULL;
}
