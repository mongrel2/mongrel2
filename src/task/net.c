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
#include <arpa/inet.h>

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

int MAX_LISTEN_BACKLOG = 128;
int SET_NODELAY = 1;

static void addr_ntop(void *sinx, char *target, int size) 
{
  struct sockaddr_in6 *sin6 = sinx;
  struct sockaddr_in *sin = sinx;

  if(sin6->sin6_family == AF_INET6) {
    inet_ntop(AF_INET6, &sin6->sin6_addr, target, size);
  } else {
    inet_ntop(AF_INET, &sin->sin_addr, target, size);
  }
}

static int cb_bind(int fd, int istcp, struct sockaddr *psa, size_t sz) 
{
    int n = 0;
    socklen_t sn = sizeof(n);
    char str[IPADDR_SIZE+1];
    int rc = 0;
#ifdef AUTHBIND_HELPER
    unsigned int port;
    char addrstr[33];
    char portstr[5];
    pid_t child;
    pid_t wp;
    int status;
#endif

    addr_ntop(psa, str, IPADDR_SIZE);
    debug("Binding to %s:%d!", str, ntohs(((struct sockaddr_in6*)psa)->sin6_port));

    /* set reuse flag for tcp */
    if(istcp && getsockopt(fd, SOL_SOCKET, SO_TYPE, (void*)&n, &sn) >= 0){
        n = 1;
        rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof n);
        check(rc != -1, "Failed to set bind socket to SO_REUSEADDR, that's messed up.");
    }

    rc = bind(fd, psa, sz);

#ifdef AUTHBIND_HELPER
    if(rc == -1) {
        // if error was EACCES then fall back to the helper

        check(errno == EACCES, "bind failed with error other than EACCESS.");

        if(psa->sa_family == AF_INET6) {
            // the last sprintf call will ensure null-termination
            for(n = 0; n < 16; ++n) {
                sprintf(addrstr + (n * 2), "%02x", (((struct sockaddr_in6 *)psa)->sin6_addr.s6_addr[n]) & 0xff);
            }
            port = ((struct sockaddr_in6 *)psa)->sin6_port;
        } else if(psa->sa_family == AF_INET) {
            sprintf(addrstr, "%08x", (((struct sockaddr_in *)psa)->sin_addr.s_addr) & 0xffffffff);
            port = ((struct sockaddr_in *)psa)->sin_port;
        }
        sprintf(portstr, "%04x", port & 0xffff);

        child = fork();
        check(child != -1, "Failed to fork for authbind-helper.");

        if(child == 0) {
            // child process
            if(dup2(fd, 0) == -1) {
                _exit(errno);
            }
            execl(AUTHBIND_HELPER, AUTHBIND_HELPER, addrstr, portstr, psa->sa_family == AF_INET6 ? "6": (char *)NULL, (char *)NULL);
            _exit(errno);
        }

        wp = waitpid(child, &status, 0);
        check(wp != -1, "waitpid failed while waiting for %s", AUTHBIND_HELPER);

        // we assume no other child process could change status right now
        // note: libauthbind makes the same assumption
        check(wp == child, "wrong child changed status while waiting for %s", AUTHBIND_HELPER);

        check(WIFEXITED(status), "unexpected status from %s", AUTHBIND_HELPER);

        if(WEXITSTATUS(status) == 0) {
            rc = 0;
        } else {
            rc = -1;
            errno = WEXITSTATUS(status);
        }

        check(rc != -1, "Failed to bind to address with %s", AUTHBIND_HELPER);
    }
#else
    check(rc != -1, "Failed to bind to address.");
#endif

    if(istcp) {
        rc = listen(fd, MAX_LISTEN_BACKLOG);
        check(rc != -1, "Failed to listen with %d backlog", MAX_LISTEN_BACKLOG);
    }

    return fd;

error:
    taskstate("bind failed");
    fdclose(fd);
    return -1;
}

static int cb_connect(int fd, int istcp, struct sockaddr *psa, socklen_t sn) 
{
    int n = 0;
    int rc = 0;

    /* for udp */
    if(!istcp) {
        n = 1;
        rc = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &n, sizeof n);
        check(rc != -1, "Failed to set SO_BROADCAST on UDP socket.");
    }

    /* start connecting */
    rc = connect(fd, psa, sn);
    check(rc != -1 || errno == EINPROGRESS, "Connect failed.");

    /* wait for finish */    
    rc = fdwait(fd, 'w');
    check(rc != -1, "Failed waiting on non-blocking connect.");

    sn = sizeof *psa;
    rc = getpeername(fd, psa, &sn);
    check_debug(rc != -1, "Connection failed, either port isn't open or peername isn't available.");

    return fd;


error:
    /* report error */
    sn = sizeof n;
    getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&n, &sn);
    errno = n == 0 ? ECONNREFUSED : n;

    fdclose(fd);
    taskstate("connect failed");

    return -1;
}

/* Get a bound or connected socket */
static int netgetsocket(int istcp, char *server, int port, 
             int is_active_open)
{
    int rc = 0;
    int fd = -1;
    int proto = istcp ? SOCK_STREAM : SOCK_DGRAM;
    char str[IPADDR_SIZE+1];
    struct addrinfo *res = NULL, *ressave = NULL;
    int passive = is_active_open && server == NULL ? 0 : AI_PASSIVE;
    int numeric = is_active_open ? 0 : AI_NUMERICHOST;

    struct addrinfo hints = {
        .ai_socktype = proto,
        .ai_flags = numeric | passive
            
    };

    char service[6] = {0}; /* 1..65535 + terminator */

    check(port >= 0 && port <= 65535, "Port %d is outsid allowed range.", port);

    debug("Attempting netgetsocket: %d, %s:%d, active: %d", 
                                istcp, server, port, 
                                is_active_open);

    /* BUG: the lookup is blocking. */
    snprintf(service, sizeof(service), "%d", port);
    service[sizeof(service) - 1] = '\0'; // make sure it is terminated

    rc = getaddrinfo(server, service, &hints, &res);
    check(rc != -1, "Get addrinfo failed for server: %s.", server);

    ressave = res;

    debug("Enumerating targets...");
    for(; res; res = res->ai_next) {
        addr_ntop(res->ai_addr, str, IPADDR_SIZE);
        debug("Trying target: %s:%d, af %d, prot %d", str, 
                  ntohs(((struct sockaddr_in6*)(res->ai_addr))->sin6_port), res->ai_family,
                  res->ai_protocol);
        
        fd = socket(res->ai_family,
                    proto,
                    res->ai_protocol);

        if(fd < 0) {
            switch(errno) {
                case EAFNOSUPPORT:
                    continue;
                case EPROTONOSUPPORT:
                    /* Address family not supported, keep trying */
                    debug("Family %d not supported", res->ai_family);
                    continue;
                default:
                    sentinel("Socket failure enumerating possible addresses.");
            }
        } else {
            fdnoblock(fd);

            // TODO: this actually tries connecting to :::1 ipv6 addresses first in many
            // cases, which is stupid since ipv4 is more likely.
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

    if(ressave) {
        freeaddrinfo(ressave);
    }

    return fd;

error:
    if(ressave) {
        freeaddrinfo(ressave);
    }
    return -1;
}

int netannounce(int istcp, char *server, int port)
{
    return netgetsocket(istcp, server, port, CB_BIND);
}

int netaccept(int fd, char *server, int *port, int enable_keepalive)
{
    int cfd = 0;
    int opt = 0;

    // welcome to hacks-central, where IPv6 and IPv4 live together in pensive harmony
    union {
        struct sockaddr_in6 ipv6;
        struct sockaddr_in ipv4;
    } addr_converter;

    socklen_t len = sizeof(addr_converter);
    int rc = 0;
   
    cfd = accept(fd, (void*)&addr_converter, &len);

    if(cfd == -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            rc = fdwait(fd, 'r');
            check(rc != -1, "Failed waiting on non-block accept.");

            cfd = accept(fd, (void*)&addr_converter, &len);
            check(cfd != -1, "Failed to accept after doing a poll on the socket.");
        } else {
            sentinel("Failed calling accept on socket that was ready: %d, %d", cfd, errno == EAGAIN);
        }
    }

    if(server) {
        if(addr_converter.ipv6.sin6_family == AF_INET) {
            check(inet_ntop(addr_converter.ipv4.sin_family, &addr_converter.ipv4.sin_addr, server, IPADDR_SIZE) != NULL,
                    "Major failure, cannot ntop ipaddresses.");
        } else {
            check(inet_ntop(addr_converter.ipv6.sin6_family, &addr_converter.ipv6.sin6_addr, server, IPADDR_SIZE) != NULL,
                    "Major failure, cannot ntop ipaddresses.");
        }
    }

    if(port) {
        if(addr_converter.ipv6.sin6_family == AF_INET) {
            *port = ntohs(addr_converter.ipv4.sin_port);
        } else {
            *port = ntohs(addr_converter.ipv6.sin6_port);
        }
    }

    fdnoblock(cfd);

    if(SET_NODELAY) {
        opt = 1;
        setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof opt);
    }

    if(enable_keepalive) {
        opt = 1;
        setsockopt(cfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&opt, sizeof opt);
    }

    taskstate("netaccept succeeded");
    return cfd;

error:
    taskstate("accept failed");
    return -1;
}


int netdial(int istcp, char *server, int port)
{
    return netgetsocket(istcp, server, port, CB_CONNECT);
}

