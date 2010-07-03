%%{
    machine State;

    import "events.h";

Proxy := (
        start: ( 
           CONNECT @proxy_connected -> Sending |
           FAILED @proxy_failed -> final |
           REMOTE_CLOSE @proxy_exit_idle |
           TIMEOUT @timeout -> final
        ),

        Connected: (
           REMOTE_CLOSE @proxy_exit_idle |
           REQ_RECV -> Routing |
           TIMEOUT @timeout -> final
        ),

        Routing: (
           HTTP_REQ PROXY @proxy_request -> Sending |
           HTTP_REQ (HANDLER|DIRECTORY) @proxy_exit_routing
        ),

        Sending: (
            REQ_SENT @proxy_req_sent -> Expecting |
            TIMEOUT @timeout -> final
        ),

        Expecting: (
            RESP_RECV @proxy_resp_recv -> Responding |
            TIMEOUT @timeout -> final
        ),

        Responding: ( 
            RESP_SENT @proxy_resp_sent -> Connected |
            TIMEOUT @timeout -> final
        )

     )  <err(error);


Connection = (
        start: ( OPEN @open -> Accepting ),

        Accepting: ( ACCEPT @accepted -> Idle ),

        Idle: (
            REQ_RECV @ident_req HTTP_REQ @route -> HTTPRouting |
            REQ_RECV @ident_req MSG_REQ @route -> MSGRouting |
            REQ_RECV @ident_req SOCKET_REQ @socket_req -> Responding |
            MSG_RESP @msg_resp -> Responding |
            CLOSE @close -> final |
            TIMEOUT @timeout -> final 
        ),

        MSGRouting: (
            HANDLER @msg_to_handler -> Queueing |
            DIRECTORY @msg_to_directory -> Responding 
        ),

        HTTPRouting: (
            HANDLER @http_to_handler -> Queueing |
            PROXY @http_to_proxy  |
            DIRECTORY @http_to_directory -> Responding
        ),

        Queueing: ( REQ_SENT @msg_sent -> Idle ),

        Delivering: (
            TIMEOUT @timeout -> final |
            REQ_SENT @req_sent -> Waiting 
        ),

        Waiting: ( 
            TIMEOUT @timeout -> final |
            RESP_RECV @resp_recv -> Responding
        ),

        Responding: (
            TIMEOUT @timeout -> final |
            RESP_SENT @resp_sent -> Idle
        )

        ) %eof(finish) <err(error);

main := (Connection)*;

}%%
