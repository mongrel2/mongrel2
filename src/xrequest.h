#include "bstring.h"
#include "handler.h"
#include "tnetstrings.h"
#include "connection.h"

typedef deliver_function (*xrequest_init_cb)(Server *srv, bstring load_path, tns_value_t *config, bstring **out_keys, size_t *out_klen);
int dispatch_extended_request(Connection *, bstring key, tns_value_t *);
int Xrequest_load(Server *srv, bstring load_path, tns_value_t *config);
