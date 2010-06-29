
#include "minunit.h"
#include <handler.h>
#include <string.h>
#include <task/task.h>

FILE *LOG_FILE = NULL;


char *test_Handler_recv_create()
{
    void *socket = Handler_recv_create("tcp://127.0.0.1:4321", "ZED");
    mu_assert(socket != NULL, "Failed to make recv socket.");
    zmq_close(socket);
    return NULL;
}

char *test_Handler_send_create()
{

    void *socket = Handler_send_create("tcp://127.0.0.1:12345", "ZED");
    mu_assert(socket != NULL, "Failed to make the send socket.");

    zmq_close(socket);

    return NULL;
}

char *test_Handler_deliver()
{
    void *socket = Handler_send_create("tcp://127.0.0.1:12346", "ZED");
    mu_assert(socket != NULL, "Failed to make the send socket.");

    char *message =  "{\"type\":\"join\"}";
    int rc = Handler_deliver(socket, 12, message, strlen(message));

    mu_assert(rc == 0, "Failed to deliver the message.");

    zmq_close(socket);
    return NULL;
}

char *test_Handler_notify_leave()
{
    void *socket = Handler_send_create("tcp://127.0.0.1:12347", "ZED");
    mu_assert(socket != NULL, "Failed to make the send socket.");

    Handler_notify_leave(socket, 100);
    
    zmq_close(socket);
    return NULL;
}


char *test_Handler_create_destroy()
{
    Handler *handler = Handler_create("tcp://127.0.0.1:12348", "ZED", "tcp://127.0.0.1:4321", "ZED");
    mu_assert(handler != NULL, "Failed to make the handler.");

    Handler_destroy(handler, 1000);

    return NULL;
}

char * all_tests() {
    mu_suite_start();
    mqinit(2);

    mu_run_test(test_Handler_send_create);
    mu_run_test(test_Handler_recv_create);
    mu_run_test(test_Handler_deliver);
    mu_run_test(test_Handler_notify_leave);
    mu_run_test(test_Handler_create_destroy);

    return NULL;
}

RUN_TESTS(all_tests);

