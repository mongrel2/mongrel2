#ifndef _pattern_h
#define _pattern_h

#include <bstring.h>

const char *pattern_match(const char *s, size_t len, const char *p);

const char *bstring_match(bstring s, bstring pattern);

#endif
