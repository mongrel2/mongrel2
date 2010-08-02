#ifndef _KEGOGI_H
#define _KEGOGI_H

#include "httpclient.h"

typedef struct Command {
    Request *request;
    Response *expected;
} Command;

#endif
