#include <filter.h>
#include <dbg.h>
#include <config/module.h>
#include <config/db.h>


int config_init(const char *path)
{
    return -1;
}

void config_close()
{
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

