#include "tnetstrings.h"
#include <string.h>
#include "dbg.h"
#include <assert.h>
#include "mem/halloc.h"

#define MAX_NUM_LEN 22 // ceil(log_10 2^64) + 2

static inline int
tns_parse_dict(void *dict, const char *data, size_t len);

static inline int
tns_parse_list(void *list, const char *data, size_t len);

static inline int
tns_render_value(void *val, tns_outbuf *outbuf);

static inline int
tns_outbuf_itoa(size_t n, tns_outbuf *outbuf);

static inline int
tns_outbuf_init(tns_outbuf *outbuf);

static inline void
tns_outbuf_free(tns_outbuf *outbuf);

static inline int
tns_outbuf_extend(tns_outbuf *outbuf);

static inline int
tns_outbuf_putc(tns_outbuf *outbuf, char c);

static inline int
tns_outbuf_rputs(tns_outbuf *outbuf, const char *data, size_t len);

static inline void
tns_inplace_reverse(char *data, size_t len);

#include "tnetstrings_impl.h"

//  Functions for parsing and rendering primitive datatypes.
static inline int tns_render_string(void *val, tns_outbuf *outbuf)
{
    tns_value_t *t = (tns_value_t *)val;
    assert(t->type == tns_tag_string && "Value is not a string.");
    return tns_outbuf_rputs(outbuf, bdata(t->value.string), blength(t->value.string));
}

static inline int tns_render_number(void *val, tns_outbuf *outbuf)
{
    tns_value_t *t = (tns_value_t *)val;
    assert(t->type == tns_tag_number && "Value is not a number.");

    long number = t->value.number;
    int negative = number < 0;

    assert(number != LONG_MIN && "LONG_MIN cannot be handled");

    while (outbuf->alloc_size < outbuf->used_size + MAX_NUM_LEN) {
        check(tns_outbuf_extend(outbuf) != -1, "Failed to extend buffer.");
    }

    if (negative) {
        number = -number;
    }

    do {
        outbuf->buffer[outbuf->used_size++] = '0' + (number%10);
    } while (number /= 10);

    if (negative) {
        outbuf->buffer[outbuf->used_size++] = '-';
    }

    return 0;

error:
    return -1;
}

static inline int tns_render_bool(void *val, tns_outbuf *outbuf)
{
    tns_value_t *t = (tns_value_t *)val;
    assert(t->type == tns_tag_bool && "Value is not a bool.");

    if(t->value.bool) {
        return tns_outbuf_rputs(outbuf, "true", 4);
    } else {
        return tns_outbuf_rputs(outbuf, "false", 5);
    }
}


static inline int tns_render_dict(void *dict, tns_outbuf *outbuf)
{
    hash_t *h = ((tns_value_t *)dict)->value.dict;
    hscan_t hs;
    hnode_t *node;
    hash_scan_begin(&hs, h);
    tns_value_t key;

    while ((node = hash_scan_next(&hs))) {
        check(tns_render_value(hnode_get(node), outbuf) == 0, "Failed to render dict value.");

        key.type = tns_tag_string;
        key.value.string = (bstring)hnode_getkey(node);

        check(tns_render_value(&key, outbuf) == 0, "Failed to render dict key.");
    }

    return 0;
error:
    return -1;
}

static inline int tns_render_list(void *list, tns_outbuf *outbuf)
{
    darray_t *L = ((tns_value_t *)list)->value.list;
    int i = 0;

    for(i = darray_end(L) - 1; i >= 0; i--) {
        tns_value_t *val = darray_get(L, i);
        check(val != NULL, "Invalid tns list structure, got a NULL.");
        check(tns_render_value(val, outbuf) == 0, "Failed to render list element.");
    }

    return 0;
error:
    return -1;
}


/** Helper functions for the tns code to work with internal data structures. */
void tns_value_destroy(tns_value_t *value)
{
    if(value == NULL) return;
    darray_t *L = value->value.list;
    int i = 0;

    switch(value->type) {
        case tns_tag_bool:
            break;
        case tns_tag_dict:
            hash_free_nodes(value->value.dict);
            hash_free(value->value.dict);
            break;
        case tns_tag_list:
            for(i = 0; i < darray_end(L); i++) {
                tns_value_destroy(darray_get(L, i));
            }
            h_free(L);
            break;
        case tns_tag_null:
            break;
        case tns_tag_number:
            break;
        case tns_tag_string:
            bdestroy(value->value.string);
            break;
        default:
            sentinel("Invalid type given: '%c'", value->type);
    }

error: // fallthrough
    free(value);
    return;
}

void tns_hnode_free(hnode_t *node, void *notused)
{
    (void)notused;

    bdestroy((bstring)hnode_getkey(node));
    tns_value_destroy(hnode_get(node));
    free(node);
}

hnode_t *tns_hnode_alloc(void *notused)
{
    (void)notused;

    return malloc(sizeof(hnode_t));
}


void* tns_parse(const char *data, size_t len, char **remain)
{
  void *val = NULL;
  char *valstr = NULL;
  tns_type_tag type = tns_tag_null;
  size_t vallen = 0;

  //  Read the length of the value, and verify that is ends in a colon.
  vallen = strtol(data, &valstr, 10);
  check(valstr != data, "Not a tnetstring: no length prefix.");
  check((valstr + vallen + 1) < (data + len) && *valstr == ':', "Not a tnetstring: invalid length prefix.");
  valstr++;

  //  Grab the type tag from the end of the value.
  type = valstr[vallen];

  //  Output the remainder of the string if necessary.
  if(remain != NULL) {
      *remain = valstr + vallen + 1;
  }

  //  Now dispatch type parsing based on the type tag.
  switch(type) {
    //  Primitive type: a string blob.
    case tns_tag_string:
        val = tns_parse_string(valstr, vallen);
        break;
    //  Primitive type: a number.
    case tns_tag_number:
        val = tns_parse_integer(valstr, vallen);
        check(val != NULL, "Not a tnetstring: invalid integer literal.");
        break;
    case tns_tag_float:
        val = tns_parse_float(valstr, vallen);
        check(val != NULL, "Not a tnetstring: invalid float literal.");
        break;
    //  Primitive type: a boolean.
    //  The only acceptable values are "true" and "false".
    case tns_tag_bool:
        if(vallen == 4 && valstr[0] == 't') {
            val = tns_get_true();
        } else if(vallen == 5 && valstr[0] == 'f') {
            val = tns_get_false();
        } else {
            sentinel("Not a tnetstring: invalid boolean literal.");
        }
        break;
    //  Primitive type: a null.
    //  This must be a zero-length string.
    case tns_tag_null:
        check(vallen == 0, "Not a tnetstring: invalid null literal.");
        val = tns_get_null();
        break;
    //  Compound type: a dict.
    //  The data is written <key><value><key><value>
    case tns_tag_dict:
        val = tns_new_dict();
        check(tns_parse_dict(val,valstr,vallen) != -1, "Not a tnetstring: broken dict items.");
        break;
    //  Compound type: a list.
    //  The data is written <item><item><item>
    case tns_tag_list:
        val = tns_new_list();
        check(tns_parse_list(val,valstr,vallen) != -1, "not a tnetstring: broken list items");
        break;
    default:
      sentinel("not a tnetstring: invalid type tag");
  }

  return val;

error:
  tns_value_destroy(val);
  return NULL;
}


void tns_outbuf_clamp(tns_outbuf *outbuf, int orig_size)
{
    size_t datalen = outbuf->used_size - orig_size;
    tns_outbuf_putc(outbuf, ':');
    tns_outbuf_itoa(datalen, outbuf);
}

char *tns_render(void *val, size_t *len)
{
  char *output = NULL;

  output = tns_render_reversed(val, len);
  check(output != NULL, "Failed to render tnetstring.");

  tns_inplace_reverse(output, *len);
  output[*len] = '\0';

  return output;

error:
  return NULL;
}

static inline void tns_render_hash_pair_list(tns_outbuf *outbuf, bstring key, struct bstrList *value)
{
    int i = 0;
    tns_value_t val = {.type = tns_tag_string};
    tns_outbuf_putc(outbuf, ']');
    int orig_size = outbuf->used_size;

    for(i = value->qty - 1; i >= 0 ; i--) {
        val.value.string = value->entry[i];
        tns_render_value(&val, outbuf);
    }

    tns_outbuf_clamp(outbuf, orig_size);
    val.value.string = key;
    tns_render_value(&val, outbuf);
}

void tns_render_hash_pair(tns_outbuf *outbuf, bstring key, bstring value)
{
    tns_value_t val = {.type = tns_tag_string, .value.string = value};
    tns_render_value(&val, outbuf);

    val.value.string = key;
    tns_render_value(&val, outbuf);
}

void tns_render_number_prepend(tns_outbuf *outbuf, long value)
{
    tns_value_t val = {.type = tns_tag_number, .value.number = value};
    tns_render_value(&val, outbuf);
}

void tns_render_string_prepend(tns_outbuf *outbuf, bstring value)
{
    tns_value_t val = {.type = tns_tag_string, .value.string = value};
    tns_render_value(&val, outbuf);
}

int tns_render_request_start(tns_outbuf *outbuf)
{
    check(tns_outbuf_init(outbuf) != -1, "Failed to init buffer.");

    check(tns_outbuf_putc(outbuf, '}') != -1, "Failed ending request.");

    return outbuf->used_size;
error:
    return -1;
}

int tns_render_request_end(tns_outbuf *outbuf, int header_start, bstring uuid, int id, bstring path)
{
    // close it off with the final size, minus ending } terminator
    tns_outbuf_clamp(outbuf, header_start);

    check(tns_outbuf_putc(outbuf, ' ') != -1, "Failed ending request.");
    check(tns_outbuf_rputs(outbuf, bdata(path), blength(path)) != -1, "Failed ending request.");

    check(tns_outbuf_putc(outbuf, ' ') != -1, "Failed ending request.");
    check(tns_outbuf_itoa(id, outbuf) != -1, "Failed ending request.");

    check(tns_outbuf_putc(outbuf, ' ') != -1, "Failed ending request.");
    check(tns_outbuf_rputs(outbuf, bdata(uuid), blength(uuid)) != -1, "Failed ending request.");

    return 0;

error:
    return -1;
}

int tns_render_log_start(tns_outbuf *outbuf)
{
    check(tns_outbuf_init(outbuf) != -1, "Failed to init buffer.");

    check(tns_outbuf_putc(outbuf, ']') != -1, "Failed ending request.");

    return outbuf->used_size;
error:
    return -1;
}

void tns_render_log_end(tns_outbuf *outbuf)
{
    tns_outbuf_clamp(outbuf, 1);
}

int tns_render_request_headers(tns_outbuf *outbuf, hash_t *headers)
{
    hscan_t scan;
    hnode_t *n = NULL;
    hash_scan_begin(&scan, headers);

    for(n = hash_scan_next(&scan); n != NULL; n = hash_scan_next(&scan)) {
        struct bstrList *val_list = hnode_get(n);
        if(val_list->qty == 0) continue;

        bstring key = (bstring)hnode_getkey(n);

        if(val_list->qty == 1) {
            tns_render_hash_pair(outbuf, key, val_list->entry[0]);
        } else {
            tns_render_hash_pair_list(outbuf, key, val_list);
        }
    }

    return 0;
}

bstring tns_outbuf_to_bstring(tns_outbuf *outbuf)
{
    if(outbuf->used_size == outbuf->alloc_size) {
        tns_outbuf_extend(outbuf);
    }

    tns_inplace_reverse(outbuf->buffer, outbuf->used_size);

    bstring b = malloc(sizeof(struct tagbstring));
    b->slen = outbuf->used_size;
    b->data = (unsigned char *)outbuf->buffer;
    b->data[b->slen] = '\0';
    b->mlen = outbuf->alloc_size;

    return b;
}

char *tns_render_reversed(void *val, size_t *len)
{
  tns_outbuf outbuf;

  check(tns_outbuf_init(&outbuf) != -1, "Failed to initialize outbuf.");

  check(tns_render_value(val, &outbuf) != -1, "Failed to render value.");
  *len = outbuf.used_size;

  if(outbuf.used_size == outbuf.alloc_size) {
      // need to extend it just a bit for the \0 terminator
      outbuf.buffer = realloc(outbuf.buffer, outbuf.used_size + 1);
      check_mem(outbuf.buffer);
  }

  return outbuf.buffer;

error:
  tns_outbuf_free(&outbuf);
  return NULL;
}


static inline int
tns_render_value(void *val, tns_outbuf *outbuf)
{
  tns_type_tag type = tns_tag_null;
  int res = -1;
  size_t datalen = 0;

  //  Find out the type tag for the given value.
  type = tns_get_type(val);
  check(type != 0, "type not serializable");

  tns_outbuf_putc(outbuf, type);
  datalen = outbuf->used_size;

  //  Render it into the output buffer, leaving space for the
  //  type tag at the end.
  switch(type) {
    case tns_tag_string:
      res = tns_render_string(val, outbuf);
      break;
    case tns_tag_number:
      res = tns_render_number(val, outbuf);
      break;
    case tns_tag_bool:
      res = tns_render_bool(val, outbuf);
      break;
    case tns_tag_null:
      res = 0;
      break;
    case tns_tag_dict:
      res = tns_render_dict(val, outbuf);
      break;
    case tns_tag_list:
      res = tns_render_list(val, outbuf);
      break;
    case tns_tag_invalid:
      sentinel("invalid tns value, can't be NULL");
      break;
    default:
      sentinel("unknown type tag: '%c'", type);
  }

  check(res == 0, "Failed to render value type: '%c'", type);

  datalen = outbuf->used_size - datalen;
  tns_outbuf_putc(outbuf, ':');
  res = tns_outbuf_itoa(datalen, outbuf);

  return res;
error:
  return -1;
}


static void
tns_inplace_reverse(char *data, size_t len)
{
  char *dend = NULL;
  char c = '\0';
  assert(data != NULL && "Data cannot be NULL.");

  dend = data + len - 1;
  while(dend > data) {
      c = *data;
      *data = *dend;
      *dend = c;
      data++;
      dend--;
  }
}

#define tns_rotate_buffer(data, remain, len, orig_len) {\
        len = len - (remain - data);\
        check(len < orig_len, "Error parsing data, buffer math is off.");\
        data = remain;\
}

static int
tns_parse_list(void *val, const char *data, size_t len)
{
    void *item = NULL;
    char *remain = NULL;
    size_t orig_len = len;

    assert(val != NULL && "Value cannot be NULL.");
    assert(data != NULL && "data cannot be NULL.");

    while(len > 0) {
        item = tns_parse(data, len, &remain);
        check(item != NULL, "Failed to parse list.");
        tns_rotate_buffer(data, remain, len, orig_len);
        check(tns_add_to_list(val, item) != -1, "Failed to add element to list.");
        item = NULL;
    }
    
    return 0;

error:
    if(item) tns_value_destroy(item);
    return -1;
}


static inline int
tns_parse_dict(void *val, const char *data, size_t len)
{
    void *key = NULL;
    void *item = NULL;
    char *remain = NULL;
    size_t orig_len = len;

    assert(val != NULL && "Value cannot be NULL.");
    assert(data != NULL && "Data cannot be NULL.");

    while(len > 0) {
        key = tns_parse(data, len, &remain);
        check(key != NULL, "Failed to parse dict key from tnetstring.");
        tns_rotate_buffer(data, remain, len, orig_len);

        item = tns_parse(data, len, &remain);
        check(item != NULL, "Failed to parse dict key from tnetstring.");

        tns_rotate_buffer(data, remain, len, orig_len);
        check(tns_add_to_dict(val,key,item) != -1, "Failed to add element to dict.");
        key = NULL;
        item = NULL;
    }

    return 0;

error:
    if(key) tns_value_destroy(key);
    if(item) tns_value_destroy(item);
    return -1;
}


static inline int
tns_outbuf_itoa(size_t n, tns_outbuf *outbuf)
{
  do {
      check(tns_outbuf_putc(outbuf, n%10+'0') != -1, "Failed to write int to tnetstring buffer.");
      n = n / 10;
  } while(n > 0);

  return 0;

error:
  return -1;
}


static inline int
tns_outbuf_init(tns_outbuf *outbuf)
{
  outbuf->buffer = malloc(64);
  check_mem(outbuf->buffer);

  outbuf->alloc_size = 64;
  outbuf->used_size = 0;
  return 0;

error:
  outbuf->alloc_size = 0;
  outbuf->used_size = 0;
  return -1;
}


static inline void
tns_outbuf_free(tns_outbuf *outbuf)
{
    if(outbuf) {
      free(outbuf->buffer);
      outbuf->buffer = NULL;
      outbuf->alloc_size = 0;
      outbuf->used_size = 0;
    }
}


static inline int
tns_outbuf_extend(tns_outbuf *outbuf)
{
  char *new_buf = NULL;
  size_t new_size = outbuf->alloc_size * 2;

  new_buf = realloc(outbuf->buffer, new_size);
  check_mem(new_buf);

  outbuf->buffer = new_buf;
  outbuf->alloc_size = new_size;

  return 0;

error:
  return -1;
}


static inline int
tns_outbuf_putc(tns_outbuf *outbuf, char c)
{
  if(outbuf->alloc_size == outbuf->used_size) {
      check(tns_outbuf_extend(outbuf) != -1, "Failed to extend buffer.");
  }

  outbuf->buffer[outbuf->used_size++] = c;

  return 0;
error:
  return -1;
}


static inline int
tns_outbuf_rputs(tns_outbuf *outbuf, const char *data, size_t len)
{
  const char *dend = NULL;
  char *buffer = NULL;

  //  Make sure we have enough room.
  while(outbuf->alloc_size - outbuf->used_size < len) {
      check(tns_outbuf_extend(outbuf) != -1, "Failed to rputs into a tnetstring buffer.");
  }

  //  Copy the data in reverse.
  buffer = outbuf->buffer + outbuf->used_size;
  dend = data + len - 1;

  while(dend >= data) {
      *buffer = *dend;
      buffer++;
      dend--;
  }

  outbuf->used_size += len;

  return 0;
error:
  return -1;
}

tns_value_t *tns_standard_table(bstring header_data, tns_value_t *rows)
{
    tns_value_t *headers = tns_parse(bdata(header_data), blength(header_data), NULL);
    tns_value_t *result = tns_new_dict();

    tns_dict_setcstr(result, "headers", headers);
    tns_dict_setcstr(result, "rows", rows);

    return result;
}
