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

#include <bstring.h>
#include <tnetstrings.h>
#include <unistd.h>
#include <config/db.h>

#include "../query_print.h"
#include "../commands.h"

int log_action(bstring db_file, bstring what, bstring why, bstring where, bstring how)
{
    int rc = 0;
    tns_value_t *res = NULL;
    bstring who = NULL;
    char *user = getlogin() == NULL ? getenv("LOGNAME") : getlogin();
    check(user != NULL, "getlogin failed and no LOGNAME env variable, how'd you do that?");
    who = bfromcstr(user);

    if(access((const char *)db_file->data, R_OK | W_OK) != 0) {
        // don't log if there's no file available
        return 0;
    }
 
    rc = DB_init(bdata(db_file));
    check(rc == 0, "Failed to open db: %s", bdata(db_file));

    if(!where) {
        char name_buf[128] = {0};
        int rc = gethostname(name_buf, 127);
        check(rc == 0, "Failed to get your machines hostname, use -where to force it.");
        where = bfromcstr(name_buf);
    }

    res = DB_exec("INSERT INTO log (who, what, location, how, why) VALUES (%Q, %Q, %Q, %Q, %Q)",
            bdata(who), bdata(what), bdata(where), bdata(how), bdata(why));
    check(res != NULL, "Failed to add log message, you're flying blind.");
    tns_value_destroy(res);

    if(biseqcstr(who, "root")) {
        log_warn("You shouldn't be running things as %s.  Use a safe user instead.", bdata(who));
    }

    bdestroy(who);
    bdestroy(where);
    DB_close();
    return 0;

error:
    if(who) bdestroy(who);
    if(where) bdestroy(where);
    if(res) tns_value_destroy(res);
    DB_close();
    return -1;
}

int Command_commit(Command *cmd)
{
    bstring what = option(cmd, "what", "");
    bstring why = option(cmd, "why", NULL);
    bstring where = option(cmd, "where", NULL);
    bstring how = option(cmd, "how", "m2sh");
    bstring db_file = option(cmd, "db", "config.sqlite");

    check_file(db_file, "config database", R_OK | W_OK);

    return log_action(db_file, what, why, where, how);

error:
    return -1;
}


int Command_log(Command *cmd)
{
    bstring db_file = option(cmd, "db", "config.sqlite");
    check_file(db_file, "config database", R_OK | W_OK);

    printf("LOG MESSAGES:\n------\n");
    return simple_query_print(db_file, "SELECT happened_at, who, location, how, what, why FROM log ORDER BY happened_at");

error:
    return -1;
}

