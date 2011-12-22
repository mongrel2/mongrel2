/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <response.h>
#include <task/task.h>
#include <dbg.h>
#include <assert.h>
#include "version.h"


// TODO: for now these are full error responses, but let people change them

struct tagbstring HTTP_100 = bsStatic("HTTP/1.1 100 Continue\r\n\r\n");

struct tagbstring HTTP_400 = bsStatic("HTTP/1.1 400 Bad Request\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 11\r\n"
    "Server: " VERSION 
    "\r\n\r\n"
    "Bad Request");

struct tagbstring HTTP_404 = bsStatic("HTTP/1.1 404 Not Found\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 9\r\n"
    "Server: " VERSION 
    "\r\n\r\n"
    "Not Found");


struct tagbstring HTTP_413 = bsStatic("HTTP/1.1 413 Request Entity Too Large\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 16\r\n"
    "Server: " VERSION 
    "\r\n\r\n"
    "Entity Too Large");

struct tagbstring HTTP_501 = bsStatic("HTTP/1.1 501 Not Implemented\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 15\r\n"
    "Server: " VERSION 
    "\r\n\r\n"
    "Not Implemented");

struct tagbstring HTTP_502 = bsStatic("HTTP/1.1 502 Bad Gateway\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 11\r\n"
    "Server: " VERSION 
    "\r\n\r\n"
    "Bad Gateway");

struct tagbstring HTTP_500 = bsStatic("HTTP/1.1 500 Internal Server Error\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 21\r\n"
    "Server: " VERSION 
    "\r\n\r\n"
    "Internal Server Error");

struct tagbstring HTTP_405 = bsStatic("HTTP/1.1 405 Method Not Allowed\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 18\r\n"
    "Server: " VERSION 
    "\r\n\r\n"
    "Method Not Allowed");


struct tagbstring HTTP_412 = bsStatic("HTTP/1.1 412 Precondition Failed\r\n"
    "Content-Type: text/plain\r\n"
    "Connection: close\r\n"
    "Content-Length: 19\r\n"
    "Server: " VERSION 
    "\r\n\r\n"
    "Precondition Failed");

struct tagbstring HTTP_417 = bsStatic("HTTP/1.1 417 Expectation Failed\r\n"
                                      "Content-Type: text/plain\r\n"
                                      "Connection: close\r\n"
                                      "Content-Length: 18\r\n"
                                      "Server: " VERSION "\r\n"
                                      "\r\n"
                                      "Expectation Failed");


struct tagbstring HTTP_304 = bsStatic("HTTP/1.1 304 Not Modified\r\n"
    "Connection: close\r\n"
    "Content-Length: 0\r\n"
    "Server: " VERSION 
    "\r\n\r\n");


struct tagbstring FLASH_RESPONSE = bsStatic("<?xml version=\"1.0\"?>"
        "<!DOCTYPE cross-domain-policy SYSTEM \"http://www.macromedia.com/xml/dtds/cross-domain-policy.dtd\">"
        "<cross-domain-policy> <allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>");

int Response_send_status(Connection *conn, bstring error)
{
    return IOBuf_send(conn->iob, bdata(error), blength(error));
}


int Response_send_socket_policy(Connection *conn)
{
    // must have +1 to include the \0 that xml sockets expect
    return IOBuf_send(conn->iob, bdata(&FLASH_RESPONSE),
                      blength(&FLASH_RESPONSE) + 1);

}


