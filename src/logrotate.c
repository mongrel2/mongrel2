
/**
 *
 * Copyright (c) 2010, Zed A. Shaw and Mongrel2 Project Contributors.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 * 
 *     * Neither the name of the Mongrel2 Project, Zed A. Shaw, nor the names
 *       of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written
 *       permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dbg.h"
#include "request.h"
#include "headers.h"
#include "setting.h"
#include "tnetstrings.h"
#include <stdio.h>
#include "zmq_compat.h"
#include <pthread.h>
#include "bstring.h"
#include <signal.h>

#define LOG_MAX 8

struct log_rotate_entry {
    bstring name;
    FILE *f;
};

static struct log_rotate_entry logs[LOG_MAX];
static size_t nlogs = 0;

int rotate_logs(void)
{
    size_t i;

    for(i=0;i<nlogs;++i) {
        check(freopen(bdata(logs[i].name),"a+",logs[i].f),"Error reopening log file: %s",bdata(logs[i].name));
        setbuf(logs[i].f, NULL);
    }
    return 0;
error:
    return -1;

}

int add_log_to_rotate_list(const bstring fname, FILE *f)
{
    check(nlogs<LOG_MAX,"Too many logs added to rotate list.");
    check(fname!=NULL,"Tried to add NULL log filename to rotate list.");
    logs[nlogs].name=bstrcpy(fname);
    logs[nlogs].f=f;
    nlogs++;
    return 0;

error:
    return -1;
}
