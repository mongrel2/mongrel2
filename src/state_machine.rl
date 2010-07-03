%%{
    machine State;

    import "events.h";

Proxy := (
        start: ( 
           CONNECT @proxy_connected -> Sending |
           FAILED @proxy_failed -> final |
           REMOTE_CLOSE @proxy_exit_idle
        ),

        Connected: (
           REMOTE_CLOSE @proxy_exit_idle |
           REQ_RECV -> Routing
        ),

        Routing: (
           HTTP_REQ PROXY @proxy_send_request -> Sending |
           HTTP_REQ (HANDLER|DIRECTORY) @proxy_exit_routing
        ),

        Sending: (
            REQ_SENT @proxy_read_response -> Expecting
        ),

        Expecting: (
            RESP_RECV @proxy_send_response -> Responding
        ),

        Responding: ( 
            RESP_SENT @proxy_parse -> Connected
        )

     )  <err(error);


Connection = (
        start: ( OPEN @open -> Accepting ),

        Accepting: ( ACCEPT @parse -> Idle ),

        Idle: (
            REQ_RECV @identify_request HTTP_REQ @route_request -> HTTPRouting |
            REQ_RECV @identify_request MSG_REQ @route_request -> MSGRouting |
            REQ_RECV @identify_request SOCKET_REQ @send_socket_response -> Responding |
            CLOSE @close -> final
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

        Queueing: ( REQ_SENT @parse -> Idle ),

        Responding: (
            RESP_SENT @parse -> Idle
        )

        ) %eof(finish) <err(error);

main := (Connection)*;

}%%
