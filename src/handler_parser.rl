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

#include "handler_parser.h"
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <bstring.h>
#include <dbg.h>

%%{

    machine HandlerParser;

    action mark { mark = fpc; }

    action length {
        char *endptr = NULL;
        target_expected_len = strtoul(mark, &endptr, 10);
        check(endptr == fpc, "Invalid length given, didn't parse correctly.");
    }

    action uuid {
        parser->uuid = blk2bstr(mark, fpc-mark);
    }

    action identifier {
        check(parser->target_count < MAX_TARGETS, "Request contains too many target listeners.");
        parser->targets[parser->target_count++] = strtoul(mark, NULL, 10); 
    }

    action targets_begin {
        targets_start = fpc;
    }

    action targets_end {
        check(fpc-targets_start == target_expected_len, 
                "Target netstring length is wrong, actual %d expected %d",
                (int)(fpc-targets_start), (int)target_expected_len);
    }

    action done { fbreak; }

    Identifier = digit+ ' '?;
    IdentList = (Identifier >mark %identifier)**;
    Length = digit+;
    UUID = (alpha | digit | '-')+;
    Targets = Length >mark %length ':' IdentList >targets_begin %targets_end ",";
    Request = UUID >mark %uuid ' ' Targets ' ' @done;

main := Request;

}%%

/** Data **/
%% write data;


/** exec **/
int HandlerParser_execute(HandlerParser *parser, const char *buffer, size_t len)
{
    const char *p = buffer;
    const char *pe = buffer+len;
    int cs = 0;
    const char *mark = p;
    const char *targets_start = NULL;
    int target_expected_len = 0;
    parser->target_count = 0;
    parser->uuid = NULL;

    %% write init;
    %% write exec noend;

    check(p <= pe, "Buffer overflow after parsing.  Tell Zed what you sent something from a handler that went %d past the end in the parser.", pe - p);

    parser->body_start = p;
    parser->body_length = pe - p;

    if ( cs == %%{ write error; }%% ) {
        return -1;
    } else if ( cs >= %%{ write first_final; }%% ) {
        return 1;
    } else {
        return 0;
    }

error:
    return -1;
}


