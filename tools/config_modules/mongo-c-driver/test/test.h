#include <stdlib.h>

#define ASSERT(x) \
    do{ \
        if(!(x)){ \
            printf("failed assert (%d): %s\n", __LINE__,  #x); \
            exit(1); \
        }\
    }while(0)

#ifdef _WIN32
#define INIT_SOCKETS_FOR_WINDOWS \
    do{ \
        WSADATA out; \
        WSAStartup(MAKEWORD(2,2), &out); \
    } while(0)
#else
#define INIT_SOCKETS_FOR_WINDOWS do {} while(0)
#endif
