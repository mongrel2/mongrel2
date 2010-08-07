#ifndef _PARAM_H
#define _PARAM_H

#include <bstring.h>
#include <adt/dict.h>

#define MAX_PARAM_COUNT 100

typedef enum ParamType {
    STRING,
    PATTERN, 
    DICT
} ParamType;

typedef struct ParamDict {
    dict_t *dict;
} ParamDict;

typedef struct Param {
    bstring name;
    ParamType type;
    union {
        bstring string;
        bstring pattern;
        ParamDict *dict;
    } data;
} Param;

Param *Param_create(bstring name, ParamType type, void *val);
Param *Param_parse(bstring name, char **pP, char **pPe);
Param *Param_copy(Param *p);
void Param_destroy(Param *p);

ParamDict *ParamDict_create();
ParamDict *ParamDict_copy();
void ParamDict_destroy(ParamDict *pd);
void ParamDict_set(ParamDict *pd, Param *p);
Param *ParamDict_get(ParamDict *pd, bstring name);

#define ParamDict_foreach(pd, p, d)                                     \
    for(d = dict_first(pd->dict), p = (d == NULL) ? NULL : dnode_get(d); \
        d != NULL;                                                      \
        d = dict_next(pd->dict, d), p = (d == NULL) ? NULL : dnode_get(d))


#endif
