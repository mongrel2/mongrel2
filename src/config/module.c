#include "config/module.h"
#include "config/db.h"

int default_init(const char *path)
{
    return DB_init(path);
}

void default_close()
{
    DB_close();
}

tns_value_t *default_load_handler(int handler_id)
{
    const char *HANDLER_QUERY = "SELECT id, send_spec, send_ident, recv_spec, recv_ident, raw_payload, protocol FROM handler WHERE id=%d";
    return DB_exec(HANDLER_QUERY, handler_id);
}

tns_value_t *default_load_proxy(int proxy_id)
{
    const char *PROXY_QUERY = "SELECT id, addr, port FROM proxy WHERE id=%d";
    return DB_exec(PROXY_QUERY, proxy_id);
}

tns_value_t *default_load_dir(int dir_id)
{
    const char *DIR_QUERY = "SELECT id, base, index_file, default_ctype, cache_ttl FROM directory WHERE id=%d";
    return DB_exec(DIR_QUERY, dir_id);
}

tns_value_t *default_load_routes(int host_id, int server_id)
{
    const char *ROUTE_QUERY = "SELECT route.id, route.path, route.target_id, route.target_type "
        "FROM route, host WHERE host_id=%d AND "
        "host.server_id=%d AND host.id = route.host_id";

    return DB_exec(ROUTE_QUERY, host_id, server_id);
}

tns_value_t *default_load_hosts(int server_id)
{
    const char *HOST_QUERY = "SELECT id, name, matching, server_id FROM host WHERE server_id = %d";
    return DB_exec(HOST_QUERY, server_id);
}

tns_value_t *default_load_server(const char *uuid)
{
    const char *SERVER_QUERY = "SELECT id, uuid, default_host, bind_addr, port, chroot, access_log, error_log, pid_file, use_ssl FROM server WHERE uuid=%Q";

    return DB_exec(SERVER_QUERY, uuid);
}


tns_value_t *default_load_mimetypes()
{
    const char *MIME_QUERY = "SELECT id, extension, mimetype FROM mimetype";
    return DB_exec(MIME_QUERY);
}

tns_value_t *default_load_settings()
{
    const char *SETTINGS_QUERY = "SELECT id, key, value FROM setting";
    return DB_exec(SETTINGS_QUERY);
}
     

ConfigModule CONFIG_MODULE = {
    .init = default_init,
    .close = default_close,
    .load_handler = default_load_handler,
    .load_proxy = default_load_proxy,
    .load_dir = default_load_dir,
    .load_routes = default_load_routes,
    .load_hosts = default_load_hosts,
    .load_server = default_load_server,
    .load_mimetypes = default_load_mimetypes,
    .load_settings = default_load_settings
};

