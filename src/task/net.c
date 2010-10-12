#include "taskimpl.h"
#include "dbg.h"
#include "server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/poll.h>
#include <stdio.h>

/* 
 * The getaddrinfo family suggests we called socket functions in a cycle 
 * until they succeed, this means we'd need to either do a flag (bind/connect)
 * to avoid excessive copypaste.
 *
 * Anyway for now the call stack is:
 *
 * netdial/netannounce -> netgetsocket -> cb_connect/cb_bind
 *
 * IPv6 code is done as per http://gsyc.escet.urjc.es/~eva/IPv6-web/ipv6.html
 */

enum {
  CB_BIND = 0,
  CB_CONNECT 
};

static void addr_ntop(void *sinx, char *target, int size) {
  struct sockaddr_in6 *sin6 = sinx;
  struct sockaddr_in *sin = sinx;
  if(sin6->sin6_family == AF_INET6) {
    inet_ntop(AF_INET6, &sin6->sin6_addr, target, size);
  } else {
    inet_ntop(AF_INET, &sin->sin_addr, target, size);
  }
}

static int 
cb_bind(int fd, int istcp, struct sockaddr *psa, size_t sz) 
{
    int n = 0;
    socklen_t sn;
    char str[IPADDR_SIZE+1];
    /* set reuse flag for tcp */
    addr_ntop(psa, str, IPADDR_SIZE);
    log_info("Binding to %s:%d!", str, ntohs(((struct sockaddr_in6*)psa)->sin6_port));
    sn = sizeof(n);
    if(istcp && getsockopt(fd, SOL_SOCKET, SO_TYPE, (void*)&n, &sn) >= 0){
        n = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof n);
    }


    if(bind(fd, psa, sz) < 0){
        taskstate("bind failed");
        fdclose(fd);
	log_err("Bind failed");
        return -1;
    }

    if(istcp)
        listen(fd, 16);
    log_info("Is TCP: %d", istcp);
    return fd;
}

static int 
cb_connect(int fd, int istcp, struct sockaddr *psa, socklen_t sn) 
{
    int n;
    char str[IPADDR_SIZE+1];

    /* for udp */
    if(!istcp){
        n = 1;
        setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &n, sizeof n);
    }
    /* start connecting */
    if(connect(fd, psa, sn) < 0 && errno != EINPROGRESS){
        taskstate("connect failed");
        log_err("Connect failed in cb_connect!");
        fdclose(fd);
        return -1;
    }

    /* wait for finish */    
    if(fdwait(fd, 'w') == -1) return -1;

    sn = sizeof *psa;
    if(getpeername(fd, psa, &sn) >= 0){
        taskstate("connect succeeded");
        return fd;
    }
    
    /* report error */
    sn = sizeof n;
    getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&n, &sn);
    if(n == 0)
        n = ECONNREFUSED;
    fdclose(fd);
    errno = n;
    taskstate("connect failed");
    errno = n;
    return -1;
}

/* Get a bound or connected socket */
static int
netgetsocket(int istcp, char *server, int port, 
             struct sockaddr_in6 *psa,
             int is_active_open)
{
    int error = 0;
    int fd = -1;
    int proto = 0;
    int result = 0;
    char str[IPADDR_SIZE+1];
    struct addrinfo hints;
    struct addrinfo *res = nil, *ressave = nil;
    struct addrinfo synt_res;
    struct sockaddr_in6 *psin6 = nil;

    char service[6]; /* 1..65535 + terminator */

    if((port <= 0) || (port >= 65535)) {
        log_err("Port %d outside of range", port);
        return -1;
    }
    log_info("Attempting netgetsocket: %d, %s:%d, active: %d", 
				istcp, server, port, 
                                is_active_open);
    memset(psa, 0, sizeof(*psa));
    memset(service, 0, sizeof(service));
    memset(&hints, 0, sizeof(hints));
    proto = istcp ? SOCK_STREAM : SOCK_DGRAM;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_socktype = proto;
    /* BUG: the lookup is blocking. */
    if((server != nil) && (strcmp(server, "*") != 0)) {
        snprintf(service, sizeof(service), "%d", port);
        error = getaddrinfo(server, service, &hints, &res);
        if(error != 0) {
            taskstate("getaddrinfo failed");
            log_err("Getaddrinfo failed");
            return -1;
        }
        ressave = res;
    } else {
        synt_res.ai_family = AF_INET6;
        synt_res.ai_protocol = 0;
        synt_res.ai_addrlen = sizeof(struct sockaddr_in6);
        synt_res.ai_addr = psa;
        synt_res.ai_next = nil;
        psin6 = psa;
        memset(psa, 0, sizeof(*psa));
        psin6->sin6_port = htons(port);
        psin6->sin6_family = AF_INET6;
        res = &synt_res;
        // Set ressave to nil so we do not try to free it later
        ressave = nil;
    }
    log_info("Enumerating targets...");
    for(; res; res = res->ai_next) {
        addr_ntop(res->ai_addr, str, IPADDR_SIZE);
        log_info("Trying target: %s:%d, af %d, prot %d", str, 
                  ntohs(((struct sockaddr_in6*)(res->ai_addr))->sin6_port), res->ai_family,
                  res->ai_protocol);
        fd = socket(res->ai_family,
                    proto,
                    res->ai_protocol);
	if(fd < 0) {
	    switch errno {
		case EAFNOSUPPORT:
		case EPROTONOSUPPORT:
		    /* Address family not supported, keep trying */
		    log_info("Family %d not supported", res->ai_family);
		    continue;
		default:
		    taskstate("socket failed");
		    break;
	    }
	} else {
	    fdnoblock(fd);
	    log_info("Trying callback");
            if(is_active_open) {
	        fd = cb_connect(fd, istcp, (void*)res->ai_addr, res->ai_addrlen);
            } else {
	        fd = cb_bind(fd, istcp, (void*) res->ai_addr, res->ai_addrlen);
            }
	    if(fd >= 0) {
		break;
	    }
	}
    }
    if(ressave) 
        freeaddrinfo(ressave);
    return fd;
}

int
netannounce(int istcp, char *server, int port)
{
    int fd = 0;
    struct sockaddr_in6 sa;

    taskstate("netannounce");
    fd = netgetsocket(istcp, server, port, &sa, CB_BIND);

    taskstate("netannounce succeeded");
    log_info("Starting server on port %d, fd = %d\n", port, fd);
    return fd;
}

int
netaccept(int fd, char *server, int *port)
{
    int cfd, one;
    struct sockaddr_in6 sa;
    uchar *ip;
    socklen_t len;
    
    if(fdwait(fd, 'r') == -1) {
        return -1;
    }

    taskstate("netaccept");
    len = sizeof sa;
    if((cfd = accept(fd, (void*)&sa, &len)) < 0){
        taskstate("accept failed");
        return -1;
    }
    if(server){
	inet_ntop(sa.sin6_family, &sa.sin6_addr, server, IPADDR_SIZE);
    }
    if(port)
        *port = ntohs(sa.sin6_port);
    fdnoblock(cfd);
    one = 1;
    setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof one);
    taskstate("netaccept succeeded");
    return cfd;
}


int
netdial(int istcp, char *server, int port)
{
    int fd;
    struct sockaddr_in6 sa;

    fd = netgetsocket(istcp, server, port, &sa, CB_CONNECT);
    
    return fd;
}

