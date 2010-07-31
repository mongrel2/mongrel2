#ifndef _fuzzrnd_h
#define _fuzzrnd_h

#include <stdlib.h>

char *FuzzRnd_data(size_t len);

void FuzzRnd_seed(char *key, size_t key_len);

#endif
