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

#include "minunit.h"
#include "cli.h"
#include <stdio.h>
#include <bstring.h>


char *test_parser() 
{
    Command cmd;
    bstring args;

    args = bfromcstr("m2sh stop -db 'config.sqlite' -name 'main'");
    mu_assert(cli_params_parse_args(args, &cmd) != -1, "Parse returned -1.");
    mu_assert(!cmd.error, "Parsing failed.");

    args = bfromcstr("m2sh stop -db 'config.sqlite' -name 'main' -now --murder");
    mu_assert(cli_params_parse_args(args, &cmd) != -1, "Parse returned -1.");
    mu_assert(!cmd.error, "Parsing failed.");

    args = bfromcstr("m2sh something -db 'config.sqlite' one two three 4 \"whatever\" --when 12");
    mu_assert(cli_params_parse_args(args, &cmd) != -1, "Parse returned -1.");
    mu_assert(!cmd.error, "Parsing failed.");

    args = bfromcstr("m2sh start -db config#sqlite -name main");
    mu_assert(cli_params_parse_args(args, &cmd) != -1, "Parse returned -1.");
    mu_assert(!cmd.error, "Parsing failed.");

    return NULL;
}

char* all_tests() {
    mu_suite_start();

    mu_run_test(test_parser);

    return NULL;
}

RUN_TESTS(all_tests);

