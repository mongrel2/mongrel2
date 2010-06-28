#ifndef _mime_h
#define _mime_h

#include <stdlib.h>

int MIME_add_type(const char *ext, const char *type);

const char *MIME_match_ext(const char *path, size_t len, const char *def);

#endif
