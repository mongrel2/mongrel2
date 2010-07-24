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

#include <setting.h>
#include <adt/tst.h>
#include <dbg.h>

static tst_t *SETTINGS_MAP = NULL;


int Setting_add(const char *key, const char *value)
{
    bstring key_str = bfromcstr(key);
    bstring value_str = bfromcstr(value);

    check(!tst_search(SETTINGS_MAP, bdata(key_str), blength(value_str)), 
            "Setting key %s already exists, can't add %s:%s",
            key, key, value);

    SETTINGS_MAP = tst_insert(SETTINGS_MAP, bdata(key_str),
            blength(key_str), value_str);

    bdestroy(key_str);

    return 0;
error:
    bdestroy(key_str);
    bdestroy(value_str);
    return -1;
}


bstring Setting_get_str(const char *key, bstring def)
{
    bstring value = tst_search(SETTINGS_MAP, key, strlen(key));

    return value == NULL ? def : value;
}

int Setting_get_int(const char *key, int def)
{
    bstring value = tst_search(SETTINGS_MAP, key, strlen(key));

    if(value) {
        return atoi((const char *)value->data);
    } else {
        return def;
    }
}

void Setting_traverse_destroy(void *value, void *data)
{
    bdestroy((bstring)value);
}

void Setting_destroy()
{
    if(SETTINGS_MAP) {
        tst_traverse(SETTINGS_MAP, Setting_traverse_destroy, NULL);
        tst_destroy(SETTINGS_MAP);
        SETTINGS_MAP = NULL;
    }
}
