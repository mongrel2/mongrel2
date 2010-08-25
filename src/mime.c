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

#include <mime.h>
#include <adt/tst.h>
#include <dbg.h>
#include "setting.h"


static tst_t *MIME_MAP = NULL;

int MAX_EXT_LEN = 0;

int MIME_add_type(const char *ext, const char *type)
{
    if(!MAX_EXT_LEN) {
        MAX_EXT_LEN = Setting_get_int("limits.mime_ext_len", 128);
        log_info("MAX limits.mime_ext_len=%d", MAX_EXT_LEN);
    }

    bstring ext_rev = bfromcstr(ext);
    bReverse(ext_rev);
    bstring type_str = bfromcstr(type);

    check(blength(ext_rev) > 0, "No zero length MIME extensions allowed: %s:%s", ext, type);
    check(blength(type_str) > 0, "No zero length MIME types allowed: %s:%s", ext, type);
    check(ext[0] == '.', "Extensions must start with a . '%s:%s'", ext, type);

    check(blength(ext_rev) < MAX_EXT_LEN, "MIME extension %s:%s is longer than %d MAX (it's %d)", ext, type, MAX_EXT_LEN, blength(ext_rev));

    check(!tst_search(MIME_MAP, bdata(ext_rev), blength(ext_rev)), 
            "MIME extension %s already exists, can't add %s:%s",
            ext, ext, type);

    MIME_MAP = tst_insert(MIME_MAP, bdata(ext_rev), blength(ext_rev), type_str);

    bdestroy(ext_rev);

    return 0;
error:
    bdestroy(ext_rev);
    bdestroy(type_str);
    return -1;
}


bstring MIME_match_ext(bstring path, bstring def)
{
    bstring type = tst_search_suffix(MIME_MAP, bdata(path), blength(path));

    return type == NULL ? def : type;
}

void MIME_traverse_destroy(void *value, void *data)
{
    bdestroy((bstring)value);
}

void MIME_destroy()
{
    if(MIME_MAP) {
        tst_traverse(MIME_MAP, MIME_traverse_destroy, NULL);
        tst_destroy(MIME_MAP);
        MIME_MAP = NULL;
    }
}
