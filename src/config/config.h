#ifndef __config_h__
#define __config_h__

typedef struct Route {
    char *path;
    char *name;
    char *style;
    char *addr;
} Route;


int config_load_routes(void *NotUsed, int argc, char **argv, char **azColName);
int config_print_routes(void *NotUsed, int argc, char **argv, char **azColName);

#endif
