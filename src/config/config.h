#ifndef __config_h__
#define __config_h__

#include <adt/list.h>

list_t *Config_load_servers(const char *path, const char *name);
int Config_load_mimetypes();

#endif
