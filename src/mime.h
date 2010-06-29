#ifndef _mime_h
#define _mime_h

#include <stdlib.h>
#include <bstring.h>

int MIME_add_type(const char *ext, const char *type);

bstring MIME_match_ext(bstring path, bstring def);

#endif
