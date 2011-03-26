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

/** Helper functions for the tns code to work with internal data structures. */

//  Functions to introspect the type of a data object.
static inline tns_type_tag tns_get_type(void *val)
{
    tns_value_t *t = (tns_value_t *)val;
    return t->type;
}

//  Functions for parsing and rendering primitive datatypes.
static inline void *tns_parse_string(const char *data, size_t len)
{
    tns_value_t *t = calloc(sizeof(tns_value_t), 1);
    t->type = tns_tag_string;
    t->value.string = blk2bstr(data, len);
    return t;
}

static inline int tns_render_string(void *val, tns_outbuf *outbuf)
{
    tns_value_t *t = (tns_value_t *)val;
    assert(t->type == tns_tag_string && "Value is not a string.");
    return tns_outbuf_rputs(outbuf, bdata(t->value.string), blength(t->value.string));
}

static inline void *tns_parse_integer(const char *data, size_t len)
{
    tns_value_t *t = calloc(sizeof(tns_value_t), 1);
    t->type = tns_tag_number;
    t->value.number = strtol(data, NULL, 10);
    return t;
}

static inline int tns_render_number(void *val, tns_outbuf *outbuf)
{
    tns_value_t *t = (tns_value_t *)val;
    char out[120] = {0};

    assert(t->type == tns_tag_bool && "Value is not a string.");

    int rc = snprintf(out, 119, "%ld", t->value.number);
    check(rc != -1 && rc <= 119, "Failed to generate number.");

    out[119] = '\0'; // safety since snprintf might not do this

    return tns_outbuf_rputs(outbuf, out, rc);

error:
    return -1;
}

static inline int tns_render_bool(void *val, tns_outbuf *outbuf)
{
    tns_value_t *t = (tns_value_t *)val;
    assert(t->type == tns_tag_bool && "Value is not a string.");

    if(t->value.bool) {
        return tns_outbuf_rputs(outbuf, "true", 4);
    } else {
        return tns_outbuf_rputs(outbuf, "false", 5);
    }
}


//  Constructors to get constant primitive datatypes.
static inline void *tns_get_null(void)
{
    tns_value_t *t = calloc(sizeof(tns_value_t), 1);
    t->type = tns_tag_null;
    return t;
}

static inline void *tns_get_true(void)
{
    tns_value_t *t = calloc(sizeof(tns_value_t), 1);
    t->type = tns_tag_bool;
    t->value.bool = 1;
    return t;
}

static inline void *tns_get_false(void)
{
    tns_value_t *t = calloc(sizeof(tns_value_t), 1);
    t->type = tns_tag_bool;
    t->value.bool = 0;
    return t;
}


//  Functions for manipulating compound datatypes.
static inline void *tns_new_dict(void)
{
    return NULL;
}

static inline void tns_free_dict(void* dict)
{
    return;
}

static inline int tns_add_to_dict(void *dict, void *key, void *item)
{
    return -1;
}

static inline int tns_render_dict(void *dict, tns_outbuf *outbuf)
{
    return -1;
}

static inline void *tns_new_list(void)
{
    return NULL;
}

static inline void tns_free_list(void* list)
{
    return;
}

static inline int tns_add_to_list(void *list, void *item)
{
    return -1;
}

static inline int tns_render_list(void *dict, tns_outbuf *outbuf)
{
    return -1;
}





void* tns_parse(const char *data, size_t len, char **remain)
{
  void *val;
  char *valstr;
  tns_type_tag type;
  size_t vallen;

  //  Read the length of the value, and verify that is ends in a colon.
  vallen = strtol(data, &valstr, 10);
  if(valstr == data) {
      log_err("not a tnetstring: no length prefix");
      return NULL;
  }
  if((valstr + vallen + 1) >= (data + len) || *valstr != ':') {
      log_err("not a tnetstring: invalid length prefix");
      return NULL;
  }
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
        if(val == NULL) {
            log_err("not a tnetstring: invalid integer literal");
        }
        break;
    //  Primitive type: a boolean.
    //  The only acceptable values are "true" and "false".
    case tns_tag_bool:
        if(vallen == 4 && strncmp(valstr, "true", 4) == 0) {
            val = tns_get_true();
        } else if(vallen == 5 && strncmp(valstr, "false", 5) == 0) {
            val = tns_get_false();
        } else {
            log_err("not a tnetstring: invalid boolean literal");
            val = NULL;
        }
        break;
    //  Primitive type: a null.
    //  This must be a zero-length string.
    case tns_tag_null:
        if(vallen != 0) {
            log_err("not a tnetstring: invalid null literal");
            val = NULL;
        } else {
            val = tns_get_null();
        }
        break;
    //  Compound type: a dict.
    //  The data is written <key><value><key><value>
    case tns_tag_dict:
        val = tns_new_dict();
        if(tns_parse_dict(val,valstr,vallen) == -1) {
            tns_free_dict(val);
            log_err("not a tnetstring: broken dict items");
            val = NULL;
        }
        break;
    //  Compound type: a list.
    //  The data is written <item><item><item>
    case tns_tag_list:
        val = tns_new_list();
        if(tns_parse_list(val,valstr,vallen) == -1) {
            tns_free_list(val);
            log_err("not a tnetstring: broken list items");
            val = NULL;
        }
        break;
    //  Whoops, that ain't a tnetstring.
    default:
      log_err("not a tnetstring: invalid type tag");
      val = NULL;
  }

  return val;
}


char *tns_render(void *val, size_t *len)
{
  char *output;
  output = tns_render_reversed(val, len);
  if(output != NULL) {
      tns_inplace_reverse(output, *len);
  }
  return output;
}


char *tns_render_reversed(void *val, size_t *len)
{
  tns_outbuf outbuf;
  
  if(tns_outbuf_init(&outbuf) == -1 ) {
      return NULL;
  }

  if(tns_render_value(val, &outbuf) == -1) {
      tns_outbuf_free(&outbuf);
  }

  *len = outbuf.used_size;
  return outbuf.buffer;
}


static int
tns_render_value(void *val, tns_outbuf *outbuf)
{
  tns_type_tag type;
  int res;
  size_t datalen;

  //  Find out the type tag for the given value.
  type = tns_get_type(val);
  if(type == 0) {
      log_err("type not serializable");
      return -1;
  }
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
      log_err("unknown type tag");
      return -1;
  }

  //  If that succeeds, write the framing info.
  if(res == 0) {
      datalen = outbuf->used_size - datalen;
      tns_outbuf_putc(outbuf, ':');
      res = tns_outbuf_itoa(datalen, outbuf);
  }
  return res;
}


static void
tns_inplace_reverse(char *data, size_t len)
{
  char *dend, c;

  dend = data + len - 1;
  while(dend > data) {
      c = *data;
      *data = *dend;
      *dend = c;
      data++;
      dend--;
  }
}



static int
tns_parse_list(void *val, const char *data, size_t len)
{
    void *item;
    char *remain;
    while(len > 0) {
        item = tns_parse(data, len, &remain);
        len = len - (remain - data);
        data = remain;
        if(item == NULL) {
            return -1;
        }
        if(tns_add_to_list(val,item) == -1) {
            return -1;
        }
    }
    return 0;
}


static int
tns_parse_dict(void *val, const char *data, size_t len)
{
    void *key, *item;
    char *remain;
    while(len > 0) {
        key = tns_parse(data, len, &remain);
        len = len - (remain - data);
        data = remain;
        if(key == NULL) {
            return -1;
        }
        item = tns_parse(data, len, &remain);
        len = len - (remain - data);
        data = remain;
        if(item == NULL) {
            return -1;
        }
        if(tns_add_to_dict(val,key,item) == -1) {
            return -1;
        }
    }
    return 0;
}


static inline int
tns_outbuf_itoa(size_t n, tns_outbuf *outbuf)
{
  do {
      if(tns_outbuf_putc(outbuf, n%10+'0') == -1) {
          return -1;
      }
      n = n / 10;
  } while(n > 0);
  return 0;
}


static inline int
tns_outbuf_init(tns_outbuf *outbuf)
{
  outbuf->buffer = malloc(64);
  if(outbuf->buffer == NULL) {
      outbuf->alloc_size = 0;
      outbuf->used_size = 0;
      return -1;
  } else {
      outbuf->alloc_size = 64;
      outbuf->used_size = 0;
      return 0;
  }
}


static inline void
tns_outbuf_free(tns_outbuf *outbuf)
{
  free(outbuf->buffer);
  outbuf->buffer = NULL;
  outbuf->alloc_size = 0;
  outbuf->used_size = 0;
}


static inline int
tns_outbuf_extend(tns_outbuf *outbuf)
{
  char *new_buf;
  size_t new_size;

  new_size = outbuf->alloc_size * 2;
  new_buf = realloc(outbuf->buffer, new_size);
  if(new_buf == NULL) {
      return -1;
  }
  outbuf->buffer = new_buf;
  outbuf->alloc_size = new_size;
  return 0;
}


static inline int
tns_outbuf_putc(tns_outbuf *outbuf, char c)
{
  if(outbuf->alloc_size == outbuf->used_size) {
      if(tns_outbuf_extend(outbuf) == -1) {
          return -1;
      }
  }
  outbuf->buffer[outbuf->used_size++] = c;
  return 0;
}


static inline int
tns_outbuf_rputs(tns_outbuf *outbuf, const char *data, size_t len)
{
  const char *dend;
  char *buffer;

  //  Make sure we have enough room.
  while(outbuf->alloc_size - outbuf->used_size < len) {
      if(tns_outbuf_extend(outbuf) == -1) {
          return -1;
      }
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
}

