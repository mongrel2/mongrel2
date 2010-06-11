#include "config.h"
#include <stdio.h>
#include <string.h>
#include "tst.h"

tst_t *routes = NULL;


int config_load_routes(void *NotUsed, int argc, char **argv, char **azColName)
{
    Route *next = calloc(sizeof(Route), 1);

    next->path = strdup(argv[0]);
    next->name = strdup(argv[1]);
    next->style = strdup(argv[2]);
    next->addr = strdup(argv[3]);

    routes = tst_insert(routes, next->path, strlen(next->path), next);

    return 0;
}

int config_print_routes(void *NotUsed, int argc, char **argv, char **azColName)
{
    Route *found = tst_search(routes, argv[0], strlen(argv[0]));

    if(!found) {
        printf("Didn't find: %s\n", argv[0]);
    } else {
        printf("Found %s, name: %s, style: %s, addr: %s\n",
                argv[0], found->name, found->style, found->addr);
    }

    return 0;
}

