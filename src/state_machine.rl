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

     )  >begin <err(proxy_error);


Connection = (
        start: ( OPEN @open -> Accepting ),

        Accepting: ( ACCEPT @accepted -> Idle ),

        Idle: (
            REQ_RECV HTTP_REQ @http_req -> HTTPRouting |
            REQ_RECV MSG_REQ @msg_req -> MSGRouting |
            REQ_RECV SOCKET_REQ @close -> final |
            MSG_RESP @msg_resp -> Responding |
            CLOSE @close -> final |
            TIMEOUT @timeout -> final 
        ),

        MSGRouting: (
            HANDLER @msg_to_handler -> Queueing |
            PROXY @msg_to_proxy -> Delivering |
            DIRECTORY @msg_to_directory -> Responding 
        ),

        HTTPRouting: (
            HANDLER @http_to_handler -> Queueing |
            PROXY @http_to_proxy  |
            DIRECTORY @http_to_directory -> Responding
        ),

        Queueing: ( REQ_SENT @req_sent -> Idle ),

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

        ) >begin %eof(finish) <err(error);

main := (Connection)*;

}%%
