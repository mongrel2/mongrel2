#include <response.h>
#include <task/task.h>
#include <dbg.h>
#include <assert.h>


// TODO: for now these are full error responses, but let people change them
struct tagbstring HTTP_404 = bsStatic("HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 9\r\n"
    "Server: Mongrel2\r\n\r\nNot Found");


struct tagbstring HTTP_413 = bsStatic("HTTP/1.1 413 Request Entity Too Large\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 16\r\n"
    "Server: Mongrel2\r\n\r\nEntity Too Large");

struct tagbstring HTTP_502 = bsStatic("HTTP/1.1 502 Bad Gateway\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 11\r\n"
    "Server: Mongrel2\r\n\r\nBad Gateway");

struct tagbstring HTTP_500 = bsStatic("HTTP/1.1 500 Internal Server Error\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 0\r\n"
    "Server: Mongrel2\r\n\r\n");

struct tagbstring HTTP_405 = bsStatic("HTTP/1.1 405 Method Not Allowed\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 0\r\n"
    "Server: Mongrel2\r\n\r\n");


struct tagbstring HTTP_412 = bsStatic("HTTP/1.1 412 Preconditioned Failed\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 0\r\n"
    "Server: Mongrel2\r\n\r\n");


struct tagbstring HTTP_304 = bsStatic("HTTP/1.1 304 Not Modified\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 0\r\n"
    "Server: Mongrel2\r\n\r\n");



struct tagbstring FLASH_RESPONSE = bsStatic("<?xml version=\"1.0\"?>"
        "<!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\">"
        "<cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>");


int Response_send_error(int fd, bstring error)
{
    return fdsend(fd, bdata(error), blength(error));
}


int Response_send_socket_policy(int fd)
{
    // must have +1 to include the \0 that xml sockets expect
    return fdsend(fd, bdata(&FLASH_RESPONSE), blength(&FLASH_RESPONSE) + 1);
}


