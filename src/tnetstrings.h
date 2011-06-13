#ifndef _tnetstrings_h
#define _tnetstrings_h

#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include "bstring.h"
#include "adt/hash.h"
#include "adt/darray.h"

typedef struct tns_outbuf_s {
  char *buffer;
  size_t used_size;
  size_t alloc_size;
} tns_outbuf;

typedef enum tns_type_tag_e {
    tns_tag_string = ',',
    tns_tag_number = '#',
    tns_tag_float = '^',
    tns_tag_bool = '!',
    tns_tag_null = '~',
    tns_tag_dict = '}',
    tns_tag_list = ']',
    tns_tag_invalid = 'Z',
} tns_type_tag;

typedef struct tns_value_t {
    tns_type_tag type;

    union {
        bstring string;
        long number;
        double fpoint;
        int bool;
        darray_t *list;
        hash_t *dict;
    } value;
} tns_value_t;

#define MAX_HASH_COUNT HASHCOUNT_T_MAX

void tns_hnode_free(hnode_t *node, void *notused);

hnode_t *tns_hnode_alloc(void *notused);

void tns_value_destroy(tns_value_t *value);

/**
*  Parse an object off the front of a tnetstring.
*  Returns a pointer to the parsed object, or NULL if an error occurs.
*  The third argument is an output parameter; if non-NULL it will
*  receive the unparsed remainder of the string.
*/
void *tns_parse(const char *data, size_t len, char** remain);

/**
*  Render an object into a string.
*  On success this function returns a malloced string containing
*  the serialization of the given object.  If the second argument
*  'len' is non-NULL it will receive the number of bytes in the string.
*  The caller is responsible for freeing the returned string.
*  On failure this function returns NULL.
*/
char *tns_render(void *val, size_t *len);

/**
*  Render an object into a string, in reverse.
*  This is just like tns_render but the output string contains the
*  rendered data in reverse.  This is actually how the internal routines
*  produce it since it involves less copying of data.  If you need to
*  copy the string off somewhere anyway, call tns_render_reversed and
*  save yourself the cost of reversing it in-place.
*/
char *tns_render_reversed(void *val, size_t *len);


void tns_render_hash_pair(tns_outbuf *outbuf, bstring key, bstring value);

int tns_render_request_headers(tns_outbuf *outbuf, hash_t *headers);

bstring tns_outbuf_to_bstring(tns_outbuf *outbuf);

void tns_outbuf_clamp(tns_outbuf *outbuf, int orig_size);


int tns_render_request_start(tns_outbuf *outbuf);

int tns_render_request_end(tns_outbuf *outbuf, int header_start, bstring uuid, int id, bstring path);

int tns_render_log_start(tns_outbuf *outbuf);

void tns_render_log_end(tns_outbuf *outbuf);

void tns_render_string_prepend(tns_outbuf *outbuf, bstring value);
void tns_render_number_prepend(tns_outbuf *outbuf, long value);

tns_value_t *tns_standard_table(bstring header_data, tns_value_t *rows);

#define tns_get_type(T) (((tns_value_t *)(T)) == NULL ? tns_tag_invalid : ((tns_value_t *)(T))->type)

#endif
