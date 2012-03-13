#include <bstring.h>
#include <filter.h>
#include <dbg.h>
#include <tnetstrings.h>
#include <version.h>
#include <dir.h>

#include <sys/stat.h>
#include <fcntl.h>

struct tagbstring ERROR_HTTP_400 = bsStatic("HTTP/1.1 400 Bad Request");
struct tagbstring ERROR_HTTP_404 = bsStatic("HTTP/1.1 404 Not Found");
struct tagbstring ERROR_HTTP_405 = bsStatic("HTTP/1.1 405 Method Not Allowed");
struct tagbstring ERROR_HTTP_412 = bsStatic("HTTP/1.1 412 Precondition Failed");
struct tagbstring ERROR_HTTP_413 = bsStatic("HTTP/1.1 413 Request Entity Too Large");
struct tagbstring ERROR_HTTP_417 = bsStatic("HTTP/1.1 417 Expectation Failed");
struct tagbstring ERROR_HTTP_500 = bsStatic("HTTP/1.1 500 Internal Server Error");
struct tagbstring ERROR_HTTP_501 = bsStatic("HTTP/1.1 501 Not Implemented");
struct tagbstring ERROR_HTTP_502 = bsStatic("HTTP/1.1 502 Bad Gateway");

struct tagbstring FORMAT = bsStatic("%s\r\n"
                                    "Content-Type: %s\r\n"
                                    "Connection: %s\r\n"
                                    "Content-Length: %d\r\n"
                                    "Server: " VERSION
                                    "\r\n\r\n");

struct tagbstring CTYPE = bsStatic("text/html");

struct tagbstring CONNECTION_STATUS = bsStatic("close");

static inline bstring get_path(int status_code, tns_value_t *config)
{
  bstring path = NULL;
  size_t len = 0;
  char *data = tns_render(config, &len);
    
  if(data != NULL) {
    bstring error_code = bformat("%d", status_code);
    tns_value_t *val = hnode_get(hash_lookup(config->value.dict, error_code));
    bdestroy(error_code);
    if(val) {
      path = bstrcpy(val->value.string);
    }
  } 
  return path;
}

static inline bstring get_error_string(int status_code)
{
  switch(status_code) {
  case 400:
    return &ERROR_HTTP_400;
  case 404:
    return &ERROR_HTTP_404;
  case 405:
    return &ERROR_HTTP_405;
  case 412:
    return &ERROR_HTTP_412;
  case 413:
    return &ERROR_HTTP_413;
  case 417:
    return &ERROR_HTTP_417;
  case 500:
    return &ERROR_HTTP_500;
  case 501:
    return &ERROR_HTTP_501;
  case 502:
    return &ERROR_HTTP_502;
  default:
    return NULL;
  }
}


static inline bstring get_header(bstring error_string, 
                                 bstring content_type, 
                                 bstring connection, 
                                 off_t content_length)
{
  return bformat(bdata(&FORMAT),
                 bdata(error_string),
                 bdata(content_type),
                 bdata(connection),
                 content_length);
}

static inline short is_error_code_supported(int error_code)
{
  if((error_code >= 400 && error_code <= 417) ||
     (error_code >= 500 && error_code <= 505)) {
    return 1;
  } else {
    return 0;
  }
}

static inline off_t file_size(bstring file_path)
{
  struct stat st;
  if(stat(bdata(file_path), &st) == 0 ) {
    return st.st_size;
  } else {
    return -1;
  }
}

static inline StateEvent send_error_page(StateEvent state, Connection *conn, tns_value_t *config)
{
  StateEvent next_state = state;
  bstring header = NULL;
  bstring error_string = NULL;
  int fd = -1;
  bstring file_path_error_page = get_path(conn->req->status_code, config);
  check(file_path_error_page != NULL, "This file path for error page is empty");

  error_string = get_error_string(conn->req->status_code);
  check(error_string != NULL, "The status code %d has no error string", conn->req->status_code);
  
  off_t error_page_file_size = file_size(file_path_error_page);
  check(error_page_file_size != -1, "The size of the file is -1");

  header = get_header(error_string,
		      &CTYPE,
		      &CONNECTION_STATUS,
		      error_page_file_size);

  fd = open(bdata(file_path_error_page), O_RDONLY);
  check(fd >= 0, "Failed to open file: %s", bdata(file_path_error_page));

  // send the header
  IOBuf_send(conn->iob, bdata(header), blength(header));
  // send the file
  IOBuf_stream_file(conn->iob, fd, error_page_file_size);
  
  if(conn->close) {
    next_state = CLOSE;
  } else {
    next_state = RESP_SENT;
  }
 error:
  if(error_string) bdestroy(error_string);
  if(file_path_error_page) bdestroy(file_path_error_page);
  if(fd >= 0) fdclose(fd);
  return next_state;
}

StateEvent filter_transition(StateEvent state, Connection *conn, tns_value_t *config)
{
  StateEvent next_state = state;
  
  if(is_error_code_supported(conn->req->status_code)) {
    next_state = send_error_page(state, conn, config);
  }

 error:
  return next_state;
}

StateEvent *filter_init(Server *srv, bstring load_path, int *out_nstates)
{
  StateEvent states[] = {HTTP_ERROR};
  *out_nstates = Filter_states_length(states);
  check(*out_nstates == 1, "Wrong state array length.");
  
  return Filter_state_list(states, *out_nstates);
  
 error:
  return NULL;
}
