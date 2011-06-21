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

#include <dbg.h>
#include <task/task.h>
#include <pattern.h>
#include "commands.h"


void taskmain(int argc, char *argv[])
{
    dbg_set_log(stderr);
    int i = 0;

    bstring arguments = bfromcstr(argv[0]);

    for(i = 1; i < argc; i++) {
        bstring a = bfromcstr(argv[i]);

        // compensate for quotes getting taken off by the shell
        // TODO: also need to escape " to bring back that
        if(bstrchr(a, ' ') != -1) {
            bcatcstr(arguments, " \"");
            bconcat(arguments, a);
            bcatcstr(arguments, "\"");
        } else {
            bcatcstr(arguments, " ");
            bconcat(arguments, a);
        }

        bdestroy(a);
    }

    debug("RUNNING: %s", bdata(arguments));
    taskexitall(Command_run(arguments));
}

