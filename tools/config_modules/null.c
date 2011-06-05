#include <filter.h>
#include <dbg.h>
#include <config/module.h>
#include <config/db.h>

FILE *LOG_FILE = NULL;

struct tagbstring GOODPATH = bsStatic("goodpath");

int config_init(const char *path)
{
    if(biseqcstr(&GOODPATH, path)) {
        log_info("Got the good path.");
        return 0;
    } else {
        log_info("Got the bad path: %s", path);
        return -1;
    }
}

void config_close()
{
    log_info("null module closed down.");
}

tns_value_t *config_load_handler(int handler_id)
{
    return NULL;
}

tns_value_t *config_load_proxy(int proxy_id)
{
    return NULL;
}

tns_value_t *config_load_dir(int dir_id)
{
    return NULL;
}

tns_value_t *config_load_routes(int host_id, int server_id)
{
    return NULL;
}

tns_value_t *config_load_hosts(int server_id)
{
    return NULL;
}

tns_value_t *config_load_server(const char *uuid)
{
    return NULL;
}


tns_value_t *config_load_mimetypes()
{
    return NULL;
}

tns_value_t *config_load_settings()
{
    return NULL;
}

