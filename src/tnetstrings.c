#undef NDEBUG
#include "tnetstrings.h"
#include "dbg.h"
#include "adt/hash.h"
#include "adt/list.h"
#include <stdarg.h>
#include <assert.h>


int tnetstring_decode(bstring payload, const char *spec, ...)
{
    return -1;
}

static inline void tnetstring_encode_null(bstring payload)
{
    bformata(payload, "%s", "0:~");
}


static inline void tnetstring_encode_cstr(bstring payload, const char *cstr)
{
    if(cstr == NULL) {
        tnetstring_encode_null(payload);
    } else {
        bformata(payload, "%d:%s,", strlen(cstr), cstr);
    }
}

static inline void tnetstring_encode_bstr(bstring payload, bstring data)
{
    if(data == NULL) {
        tnetstring_encode_null(payload);
    } else {
        bformata(payload, "%d:%s,", blength(data), bdata(data));
    }
}

static inline void tnetstring_encode_bool(bstring payload, int data)
{
    int length = data == 0 ? 5 : 4;
    const char *content = data == 0 ? "false" : "true";

    bformata(payload, "%d:%s!", length, content);
}

static inline void tnetstring_encode_list(bstring payload, struct bstrList *data)
{
    if(data == NULL) {
        tnetstring_encode_null(payload);
    } else {
        int i = 0;
        bstring out = bfromcstr("");

        for(i = 0; i < data->qty; i++) {
            tnetstring_encode_bstr(out, data->entry[i]);
        }

        bformata(payload, "%d:%s]", blength(out), bdata(out));
    }
}


static inline void tnetstring_encode_hash(bstring payload, hash_t *data)
{
    if(data == NULL) {
        tnetstring_encode_null(payload);
    } else {
        bstring out = bfromcstr("");
        hscan_t scan;
        hnode_t *i = NULL;
        hash_scan_begin(&scan, data);

        for(i = hash_scan_next(&scan); i != NULL; i = hash_scan_next(&scan))
        {
            struct bstrList *val_list = hnode_get(i);
            tnetstring_encode_bstr(out, (bstring)hnode_getkey(i));

            if(val_list->qty > 1) {
                tnetstring_encode_list(out, val_list);
            } else {
                tnetstring_encode_bstr(out, val_list->entry[0]);
            }
        }

        bformata(payload, "%d:%s}", blength(out), bdata(out));
    }
}

static inline void tnetstring_encode_digit(bstring payload, long number)
{
    bstring data = bformat("%d", number);
    bformata(payload, "%d:%s#", blength(data), bdata(data));
    bdestroy(data);
}

int tnetstring_encode(bstring payload, const char *spec, ...)
{
    va_list argp;
    const char *p = NULL;
    va_start(argp, spec);

    for(p = spec; *p != '\0'; p++) {
        if(*p == '%') { 
            switch(*++p) {
                case '!':
                    tnetstring_encode_cstr(payload, va_arg(argp, const char *));
                    break;
                case 's':
                    tnetstring_encode_bstr(payload, va_arg(argp, bstring));
                    break;
                case 'd':
                    tnetstring_encode_digit(payload, va_arg(argp, long));
                    break;
                case 'h':
                    tnetstring_encode_hash(payload, va_arg(argp, hash_t *));
                    break;
                case 'n':
                    tnetstring_encode_null(payload);
                    break;
                case 'l':
                    tnetstring_encode_list(payload, va_arg(argp, struct bstrList *));
                    break;
                case 'b':
                    tnetstring_encode_bool(payload, va_arg(argp, int));
                    break;
                case '%':
                    bconchar(payload, '%');
                    break;
                default:
                    sentinel("Invalid specifier char.");
            }
        } else {
            bconchar(payload, *p);
        }
    }

    va_end(argp);
    return 0;

error:
    va_end(argp);
    return -1;
}

