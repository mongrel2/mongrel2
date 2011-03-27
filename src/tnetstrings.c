#include "tnetstrings.h"
#include <string.h>
#include "dbg.h"
#include <assert.h>


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


char *tns_render(void *val, size_t *len)
{
  char *output = NULL;

  output = tns_render_reversed(val, len);
  check(output != NULL, "Failed to render tnetstring.");

  tns_inplace_reverse(output, *len);

  return output;

error:
  return NULL;
}


char *tns_render_reversed(void *val, size_t *len)
{
  tns_outbuf outbuf;

  check(tns_outbuf_init(&outbuf) != -1, "Failed to initialize outbuf.");

  check(tns_render_value(val, &outbuf) != -1, "Failed to render value.");
  *len = outbuf.used_size;

  return outbuf.buffer;

error:
  tns_outbuf_free(&outbuf);
  return NULL;
}


static int
tns_render_value(void *val, tns_outbuf *outbuf)
{
  tns_type_tag type = tns_tag_null;
  int res = -1;
  size_t datalen = 0;

  //  Find out the type tag for the given value.
  type = tns_get_type(val);
  check(type != 0, "type not serializable");

  tns_outbuf_putc(outbuf,type);
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

    assert(value != NULL && "Value cannot be NULL.");
    assert(data != NULL && "data cannot be NULL.");

    while(len > 0) {
        item = tns_parse(data, len, &remain);
        check(item != NULL, "Failed to parse list.");
        tns_rotate_buffer(data, remain, len, orig_len);
        check(tns_add_to_list(val, item) != -1, "Failed to add element to list.");
    }
    
    return 0;

error:
    if(item) tns_value_destroy(item);
    return -1;
}


static int
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
    }

    return 0;

error:
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

