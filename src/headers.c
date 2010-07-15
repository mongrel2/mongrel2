#include <headers.h>

struct tagbstring HTTP_CONTENT_LENGTH = bsStatic("Content-Length");
struct tagbstring HTTP_HOST = bsStatic("Host");
struct tagbstring HTTP_METHOD = bsStatic("METHOD");
struct tagbstring HTTP_VERSION = bsStatic("VERSION");
struct tagbstring HTTP_URI = bsStatic("URI");
struct tagbstring HTTP_PATH = bsStatic("PATH");
struct tagbstring HTTP_QUERY = bsStatic("QUERY");
struct tagbstring HTTP_FRAGMENT = bsStatic("FRAGMENT");
struct tagbstring HTTP_BODY = bsStatic("BODY");
struct tagbstring JSON_METHOD = bsStatic("JSON");

struct tagbstring HTTP_IF_MATCH = bsStatic("If-Match");
struct tagbstring HTTP_IF_NONE_MATCH = bsStatic("If-None-Match");
struct tagbstring HTTP_IF_MODIFIED_SINCE = bsStatic("If-Modified-Since");
struct tagbstring HTTP_IF_UNMODIFIED_SINCE = bsStatic("If-Unmodified-Since");


struct tagbstring HTTP_POST = bsStatic("POST");
struct tagbstring HTTP_GET = bsStatic("GET");
struct tagbstring HTTP_HEAD = bsStatic("HEAD");
struct tagbstring HTTP_DELETE = bsStatic("DELETE");
struct tagbstring HTTP_PUT = bsStatic("PUT");
struct tagbstring HTTP_OPTIONS = bsStatic("OPTIONS");
