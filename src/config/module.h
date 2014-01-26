#ifndef _module_h
#define _module_h

#include "tnetstrings.h"

typedef struct ConfigModule {
    int (*init)(const char *path);
    void (*close)();
    tns_value_t *(*load_handler)(int handler_id);
    tns_value_t *(*load_proxy)(int proxy_id);
    tns_value_t *(*load_dir)(int dir_id);
    tns_value_t *(*load_routes)(int host_id, int server_id);
    tns_value_t *(*load_hosts)(int server_id);
    tns_value_t *(*load_filters)(int server_id);
    tns_value_t *(*load_xrequests)(int server_id);
    tns_value_t *(*load_server)(const char *uuid);
    tns_value_t *(*load_mimetypes)();
    tns_value_t *(*load_settings)();
} ConfigModule;

extern ConfigModule CONFIG_MODULE;

#endif
