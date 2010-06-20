#ifndef _pattern_h
#define _pattern_h

#include <adt/tst.h>
#include <adt/list.h>

const char *pattern_match(const char *s, size_t len, const char *p);

list_t *pattern_tst_collect(tst_t *from, const char *pattern);

#endif
