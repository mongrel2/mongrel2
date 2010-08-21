#include "param.h"

#include <stdlib.h>

#include <adt/dict.h>
#include <bstring.h>
#include <dbg.h>

Param *Param_create(bstring name, ParamType type, void *val) {
    check(name != NULL && val != NULL, "Null name or val passed to Param_create");
    Param *p = calloc(sizeof(*p), 1);
    check_mem(p);
    p->name = name;
    p->type = type;
    switch(p->type) {
        case DICT:
            p->data.dict = (ParamDict *)val;
            break;
        case STRING:
            p->data.string = (bstring)val;
            break;
        case PATTERN:
            p->data.pattern = (bstring)val;
            break;
    }
    return p;

error:
    Param_destroy(p);
    return NULL;
}

Param *Param_copy(Param *p) {
    if(!p) return NULL;
    Param *p2 = calloc(sizeof(*p2), 1);
    check_mem(p2);

    p2->name = bstrcpy(p->name);
    check_mem(p2->name);
    p2->type = p->type;

    switch(p->type) {
        case DICT:
            p2->data.dict = ParamDict_copy(p->data.dict);
	    check_mem(p2->data.dict);
            break;
        case STRING:
            p2->data.string = bstrcpy(p->data.string);
	    check_mem(p2->data.string);
            break;
        case PATTERN:
            p2->data.pattern = bstrcpy(p->data.pattern);
	    check_mem(p2->data.pattern);
            break;
    }

    return p2;

error:
    Param_destroy(p2);
    return NULL;
}

void Param_destroy(Param *p) {
    if(p) {
        if(p->name) bdestroy(p->name);
        switch(p->type) {
            case DICT:
                if(p->data.dict) ParamDict_destroy(p->data.dict);
                break;
            case STRING:
                if(p->data.string) bdestroy(p->data.string);
                break;
            case PATTERN:
                if(p->data.pattern) bdestroy(p->data.pattern);
                break;
        }
        free(p);
    }
}

static dnode_t *pd_alloc_dict(void *notused) {
    return (dnode_t *)calloc(sizeof(dnode_t), 1);
}

static void pd_free_dict(dnode_t *node, void *notused) {
    // We don't have to destroy the key, because it's in Param
    Param_destroy((Param *)dnode_get(node));
    free(node);
}

ParamDict *ParamDict_create() {
    ParamDict *pd = calloc(sizeof(*pd), 1);
    check_mem(pd);
    pd->dict = dict_create(MAX_PARAM_COUNT, (dict_comp_t) bstricmp);
    check_mem(pd->dict);

    dict_set_allocator(pd->dict, pd_alloc_dict, pd_free_dict, NULL);
    // Do not allow dupes

    return pd;

error:
    ParamDict_destroy(pd);
    return NULL;
}

ParamDict *ParamDict_copy(ParamDict *pd) {
    if(!pd) return NULL;

    ParamDict *pd2 = ParamDict_create();
    dnode_t *d;
    Param *p, *p2;

    check_mem(pd2);
    ParamDict_foreach(pd, p, d) {
        p2 = Param_copy(p);
        check_mem(p2);
        ParamDict_set(pd2, p2);
    }

    return pd2;

error:
    ParamDict_destroy(pd);
    return NULL;
}

void ParamDict_destroy(ParamDict *pd) {
    if(pd) {
        if(pd->dict) {
            dict_free_nodes(pd->dict);
            dict_destroy(pd->dict);
        }
        free(pd);
    }
}

void ParamDict_set(ParamDict *pd, Param *p) {
    if(!pd || !p) return;

    dnode_t *node = dict_lookup(pd->dict, p->name);
    if(node) dict_delete_free(pd->dict, node);

    dict_alloc_insert(pd->dict, p->name, p);

    return;
}

Param *ParamDict_get(ParamDict *pd, bstring name) {
    if(!pd || !name) return NULL;
    
    dnode_t *node = dict_lookup(pd->dict, name);
    return node == NULL ? NULL : dnode_get(node);
}
