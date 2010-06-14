#include <zmq.h>
#include <string.h>
#include <stdio.h>

void our_free(void *data, void *hint)
{
    free(data);
}

int main(int argc, char *argv[])
{
    void *ctx, *s;
    zmq_msg_t query, resultset;
    char buffer[1024];

    if(argc != 2) {
        printf("usage: mqshell tcp://127.0.0.1:9999\n");
        return 1;
    }

    /* Initialise 0MQ context, requesting a single application thread
       and a single I/O thread */
    ctx = zmq_init (1);

    /* Create a ZMQ_REP socket to receive requests and send replies */
    s = zmq_socket (ctx, ZMQ_REQ);

    printf("connecting\n");
    zmq_connect (s, argv[1]);

    while(1) {
        printf("> ");

        if(!fgets(buffer, sizeof(buffer) - 1, stdin)) {
            printf("bye\n");
            return 0;
        }

        zmq_msg_init_data (&query, strdup(buffer), strlen(buffer) + 1, our_free, NULL);

        zmq_send (s, &query, 0);

        zmq_msg_init (&resultset);

        zmq_msg_close (&query);

        zmq_recv (s, &resultset, 0);

        printf(">>> %.*s\n", zmq_msg_size(&resultset), zmq_msg_data(&resultset));
        zmq_msg_close (&resultset);
    }
}

