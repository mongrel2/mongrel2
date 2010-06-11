#include <zmq.h>
#include <string.h>
#include <stdio.h>

void our_free(void *data, void *hint)
{
    free(data);
}

int main () 
{
    void *ctx, *s;
    zmq_msg_t query, resultset;

    /* Initialise 0MQ context, requesting a single application thread
       and a single I/O thread */
    ctx = zmq_init (1);

    /* Create a ZMQ_REP socket to receive requests and send replies */
    s = zmq_socket (ctx, ZMQ_REQ);

    printf("connecting\n");
    zmq_connect (s, "tcp://127.0.0.1:9999");

    while(1) {
        /* Allocate an empty message to receive a query into */
        zmq_msg_init_data (&query, strdup("HELLO"), strlen("HELLO") + 1, our_free, NULL);

        /* Receive a message, blocks until one is available */
        printf("send\n");
        zmq_send (s, &query, 0);

        /* Allocate a message for sending the resultset */
        zmq_msg_init (&resultset);

        /* TODO: Process the query here and fill in the resultset */

        /* Deallocate the query */
        zmq_msg_close (&query);

        /* Send back our canned response */
        printf("receive\n");
        zmq_recv (s, &resultset, 0);
        zmq_msg_close (&resultset);
    }
}

