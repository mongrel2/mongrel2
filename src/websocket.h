#ifndef _websocket_h
#define _websocket_h

#include <bstring.h>

extern struct tagbstring WS_REQ_METHOD;
extern struct tagbstring WS_CONNECTION;
extern struct tagbstring WS_UPGRADE;
extern struct tagbstring WS_WEBSOCKET;
extern struct tagbstring WS_HOST;
extern struct tagbstring WS_SEC_WS_KEY;
extern struct tagbstring WS_SEC_WS_VER;
extern struct tagbstring WS_FLAGS;

static inline int64_t Websocket_packet_length(const unsigned char *data, int avail)
{
    uint64_t len;
    unsigned mask;

    if (avail < 2) return -1;
    len = (data[1] & 0x7f);
    mask = ((data[1] & 0x80) != 0);

    if (len<126) {
        len = len+2;
    }
    else if (len == 126) {
       if (avail < 4) return -1;
       len = (data[2] << 8) | data[3];
       len +=4;
    }
    else {
        if (avail < 10) return -1;
        if (data[2]&0x80) return -2;
        len  = ((uint64_t) data[2]) << 56 |
            ((uint64_t) data[3]) << 48 |
            ((uint64_t) data[4]) << 40 |
            ((uint64_t) data[5]) << 32 |
            ((uint64_t) data[6]) << 24 |
            ((uint64_t) data[7]) << 16 |
            ((uint64_t) data[8]) << 8  |
            ((uint64_t) data[9]);
        len +=10;
    }
    if(mask) len+=4;
    if(len&0x8000000000000000ULL) return -2;
    return len;
}

static inline int Websocket_header_length(const unsigned char *data, int avail)
{
    uint64_t len;
    int header_size=0;
    unsigned mask;

    if (avail < 2) return -1;
    len = (data[1] & 0x7f);
    mask = ((data[1] & 0x80) != 0);

    if (len<126) {
        header_size=2;
    }
    else if (len == 126) {
       if (avail < 4) return -1;
       header_size=4;
    }
    else {
        if (avail < 10) return -1;
        if (data[2]&0x80) return -2;
        header_size=10;
    }
    if(mask) header_size+=4;
    return header_size;
}

static inline int WS_mask(const unsigned char *header) {
    return (header[1] & 0x80) != 0;
}

static inline int WS_fin(const unsigned char *header) {
    return (header[0] & 0x80) != 0;
}

static inline int WS_is_control(const unsigned char *header) {
    return (header[0] & 0x8) != 0;
}

static inline int WS_is_continuation(const unsigned char *header) {
    return (header[0] & 0xf) == 0;
}

/* TODO This is a bit long for inline, move to websocket.c*/
static inline const char * WS_validate_packet(const unsigned char * header,int continuationExpected) {

    if (continuationExpected && !(WS_is_control(header)|WS_is_continuation(header)))
        return "Received websocket data packet in middle of fragmented message.";
    if (WS_is_continuation(header) && !continuationExpected)
        return "Received spurious continuation message.";
    if (!WS_mask(header))
        return "Received unmasked WS packet.";
    if (!WS_fin(header) && WS_is_control(header))
        return "Received fragmented control packet.";
    if(WS_is_control(header) && ((header[1]&0x7f) >= 126))
        return "Too long control packet.";
    return NULL;
}

bstring websocket_make_header(unsigned char flags,uint64_t length, int masked,unsigned char key[]);

bstring websocket_challenge(bstring input);

enum Ws_Flags {
 WSFLAG_CONN=1,
 WSFLAG_UPGRADE=2,
 WSFLAG_HOST=4,
 WSFLAG_SEC_KEY=8,
 WSFLAG_SEC_VER=16
};
#endif

