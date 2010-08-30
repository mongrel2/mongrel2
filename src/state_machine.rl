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

%%{
    machine State;

    import "events.h";

Proxy := (
        start: ( 
           CONNECT @proxy_deliver -> Sending |
           FAILED @proxy_failed -> Closing
        ),

        Proxying: (
            HTTP_REQ @proxy_deliver -> Sending |
            PROXY @proxy_exit_routing |
            HANDLER @proxy_exit_routing |
            DIRECTORY @proxy_exit_routing |
            REMOTE_CLOSE @proxy_close -> Closing
        ),

        Sending: (
            REQ_SENT @proxy_reply_parse -> Receiving |
            REMOTE_CLOSE @proxy_close -> Closing
        ),

        Receiving: (
            REQ_RECV @proxy_req_parse -> Proxying |
            REMOTE_CLOSE @proxy_close -> Closing
        ),

        Closing: (
            CLOSE @proxy_exit_idle
        )

     )  <err(error);


Connection = (
        start: ( OPEN @open -> Accepting ),

        Accepting: ( ACCEPT @parse -> Identifying ),

        Identifying: (
            REQ_RECV @register_request -> Registered |
            CLOSE @close -> final
        ),

        Registered: (
            HTTP_REQ @route_request -> HTTPRouting |
            MSG_REQ @route_request -> MSGRouting |
            SOCKET_REQ @send_socket_response -> Responding |
            CLOSE @close -> final
        ),

        Idle: (
            REQ_RECV @identify_request HTTP_REQ @route_request -> HTTPRouting |
            REQ_RECV @identify_request MSG_REQ @route_request -> MSGRouting |
            REQ_RECV @identify_request SOCKET_REQ @send_socket_response -> Responding |
            CLOSE @close -> final
        ),

        MSGRouting: ( HANDLER @msg_to_handler -> Queueing ),

        HTTPRouting: (
            HANDLER @http_to_handler -> Queueing |
            PROXY @http_to_proxy  |
            DIRECTORY @http_to_directory -> Responding |
            CLOSE @close -> final
        ),

        Queueing: ( REQ_SENT @parse -> Idle ),

        Responding: (
            RESP_SENT @parse -> Idle |
            CLOSE @close -> final
        )

    ) %eof(finish) <err(error);

main := (Connection)*;

}%%
