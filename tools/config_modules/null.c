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

#include <filter.h>
#include <dbg.h>
#include <config/module.h>
#include <config/db.h>

struct tagbstring GOODPATH = bsStatic("goodpath");

int config_init(const char *path)
{
    if(biseqcstr(&GOODPATH, path)) {
        log_info("Got the good path.");
        return 0;
    } else {
        log_info("Got the bad path: %s", path);
        return -1;
    }
}

void config_close()
{
}

tns_value_t *config_load_handler(int handler_id)
{
    return NULL;
}

tns_value_t *config_load_proxy(int proxy_id)
{
    return NULL;
}

tns_value_t *config_load_dir(int dir_id)
{
    return NULL;
}

tns_value_t *config_load_routes(int host_id, int server_id)
{
    return NULL;
}

tns_value_t *config_load_hosts(int server_id)
{
    return NULL;
}

tns_value_t *config_load_server(const char *uuid)
{
    return NULL;
}


tns_value_t *config_load_mimetypes()
{
    return NULL;
}

tns_value_t *config_load_settings()
{
    return NULL;
}

tns_value_t *config_load_filters(int server_id)
{
    return NULL;
}

